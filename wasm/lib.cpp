#include "util.hpp"
#include "type.hpp"
#include "value.hpp"

#include <emscripten/bind.h>
#include <stdexcept>
#include <string>

#include "siplus/context.hxx"

std::shared_ptr<SIPlus::InvocationContext> get_context_from_opts(
    std::shared_ptr<SIPlus::SIPlusParserContext> context, 
    const emscripten::val& val
) {
    assert_typeof("opts", val, "object");

    auto builder = context->builder();

    if(!val.hasOwnProperty("default")) {
        throw std::runtime_error{"Must specify default data"};
    }

    builder.use_default(jsToCpp(val["default"]));

    if(val.hasOwnProperty("extra")) {
        auto extra = val["extra"];
        assert_typeof("opts.extra", extra, "object");

        auto keys = emscripten::val::global("Object").call<emscripten::val>("keys", extra);
        for(int i = 0; i < keys["length"].as<long>(); i++) {
            auto key = keys[i];
            assert_typeof("key of ", key, "string");

            auto value = extra[key];
            builder.with(key.as<std::string>(), jsToCpp(extra[key]));
        }
    }

    return builder.build();
}

SIPlus::ParseOpts get_opts_from_val(emscripten::val value) {
    SIPlus::ParseOpts opts;

    if(value.hasOwnProperty("globals")) {
        auto globals = value["globals"];
        assert_typeof("opts.globals", globals, "object");

        auto length = globals["length"];
        assert_typeof("opts.globals.length", length, "number");

        for(int i = 0; i < length.as<long>(); i++) {
            auto name = globals[i];
            assert_typeof(SIPlus::util::to_string("opts.globals[", i, "]"), name, "string");

            opts.globals.push_back(name.as<std::string>());
        }
    }

    return opts;
}

class EM_TextConstructor {
public:
    EM_TextConstructor(
        std::shared_ptr<SIPlus::SIPlusParserContext> context,
        SIPlus::text::TextConstructor constructor
    ) : context_(context), constructor_(constructor) {}

    std::string
    construct(emscripten::val value) {
        auto context = get_context_from_opts(context_, value);
        return constructor_.construct_with(context);
    }

private:
    SIPlus::text::TextConstructor constructor_;
    std::shared_ptr<SIPlus::SIPlusParserContext> context_;
};

class EM_ValueRetriever {
public:
    EM_ValueRetriever(
        std::shared_ptr<SIPlusParserContext> context,
        std::shared_ptr<ValueRetriever> retriever
    ) : retriever_(retriever), context_(context) {}

    emscripten::val
    retrieve(emscripten::val value) {
        auto context = get_context_from_opts(context_, value);
        auto result = retriever_->retrieve(*context);

        if(!result.is<JSType>()) {
            result = context_->convert<JSType>(result);
        }

        return result.as<JSType>();
    }

private:
    std::shared_ptr<ValueRetriever> retriever_;
    std::shared_ptr<SIPlusParserContext> context_;
};

class EM_SIParserContext {
public:
    EM_SIParserContext(std::shared_ptr<SIPlus::SIPlusParserContext> context)
        : context_(context) {}

    void emplace_function(emscripten::val name, emscripten::val impl) {
        assert_typeof("name", name, "string");
        assert_typeof("impl", impl, "function");

        context_->emplace_function<JsFunctionImpl>(name.as<std::string>(), context_, impl);
    }

private:
    std::shared_ptr<SIPlus::SIPlusParserContext> context_;
};

class EM_SIParser {
public:
    EM_SIParser() : parser_() {
        auto context = parser_.context().shared_from_this(); 
        context->emplace_converter<JsArrayConverter>(context);
        context->emplace_converter<ToJsPrimitiveConverter>();
        context->emplace_converter<FromJsPrimitiveConverter>(context);

        SIPlus::stl::attach_stl(*context);
    }

    EM_ValueRetriever
    get_expression(emscripten::val text, emscripten::val opts) {
        if(!text.isString()) {
            throw std::runtime_error{"expected first argument to be string. Got " + 
                text.typeOf().as<std::string>()};
        }

        return EM_ValueRetriever{
            parser_.context().shared_from_this(),
            parser_.get_expression(text.as<std::string>(), get_opts_from_val(opts))
        };
    }

    EM_TextConstructor
    get_interpolated(emscripten::val text, emscripten::val opts) {
        if(!text.isString()) {
            throw std::runtime_error{"expected first argument to be string. Got " + 
                text.typeOf().as<std::string>()};
        }

        return EM_TextConstructor{
            parser_.context().shared_from_this(),
            parser_.get_interpolation(text.as<std::string>(), get_opts_from_val(opts))
        };
    }

    EM_SIParserContext
    context() {
        return parser_.context().shared_from_this();;
    }

private:
    SIPlus::Parser parser_;
};

std::string getExceptionMessage(int eptr) {
    return reinterpret_cast<std::exception*>(eptr)->what();
}

EMSCRIPTEN_BINDINGS(siplus) {
    emscripten::function("siGetExceptionMessage", &getExceptionMessage);

    auto parser_class = emscripten::class_<EM_SIParser>("SIPlus")
        .constructor()
        .function("parse_interpolated", &EM_SIParser::get_interpolated)
        .function("parse_expression", &EM_SIParser::get_expression)
        .function("context", &EM_SIParser::context);

    auto context_class = emscripten::class_<EM_SIParserContext>("ParserContext")
        .function("emplace_function", &EM_SIParserContext::emplace_function);

    auto retriever_class = emscripten::class_<EM_ValueRetriever>("ValueRetriever")
        .function("retrieve", &EM_ValueRetriever::retrieve);

    auto constructor_class = emscripten::class_<EM_TextConstructor>("TextConstructor")
        .function("construct", &EM_TextConstructor::construct);
}
