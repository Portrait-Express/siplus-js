#include <emscripten/bind.h>
#include <stdexcept>
#include <string>

#include "siplus/context.h"
#include "siplus/function.h"
#include "siplus/parser.h"
#include "siplus/text/constructor.h"
#include "siplus/text/data.h"
#include "siplus/text/value_retrievers/retriever.h"
#include "siplus/util.h"

#include "stdlib.h"

void assert_typeof(const std::string& name, const emscripten::val& val, const std::string& type) {
    if(val.typeOf().as<std::string>() != type) {
        auto msg = SIPlus::util::to_string(
            "Expected ", name, " to be type ", type, 
            " got ", val.typeOf().as<std::string>());

        throw std::runtime_error{msg};
    }
}

std::shared_ptr<SIPlus::InvocationContext> get_context_from_opts(
    std::shared_ptr<SIPlus::SIPlusParserContext> context, 
    const emscripten::val& val
) {
    assert_typeof("opts", val, "object");

    auto builder = context->builder();

    if(!val.hasOwnProperty("default")) {
        throw std::runtime_error{"Must specify default data"};
    }

    builder.use_default(decay(val["default"]));

    if(val.hasOwnProperty("extra")) {
        auto extra = val["extra"];
        assert_typeof("opts.extra", extra, "object");

        auto keys = emscripten::val::global("Object").call<emscripten::val>("keys");
        for(int i = 0; i < keys["length"].as<long>(); i++) {
            auto key = keys[i];
            assert_typeof("key of ", key, "string");

            auto value = extra[key];
            builder.with(key.as<std::string>(), decay(extra[key]));
        }
    }

    return builder.build();
}

SIPlus::ParseOpts get_opts_from_val(emscripten::val value) {
    SIPlus::ParseOpts opts;

    if(value.hasOwnProperty("globals")) {
        auto globals = value["globals"];
        assert_typeof("opts.globals", globals, "objects");

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

struct JsFunctionValueRetriever : SIPlus::text::ValueRetriever {
    JsFunctionValueRetriever(
        std::weak_ptr<SIPlus::SIPlusParserContext> context,
        std::shared_ptr<SIPlus::text::ValueRetriever> parent,
        std::vector<std::shared_ptr<SIPlus::text::ValueRetriever>> parameters,
        emscripten::val impl
    ) : context_(context), parent_(parent), parameters_(parameters), impl_(impl) {
        assert_typeof("function_impl", impl_, "function");
    }

    SIPlus::text::UnknownDataTypeContainer retrieve(
        SIPlus::InvocationContext& value
    ) const override {
        auto ctx = context_.lock();
        auto arr = emscripten::val::global("Array").new_(parameters_.size() + 2);

        //Base value
        arr.set(0, ctx->convert<emscripten::val>(value.default_data()).as<emscripten::val>());

        //Parent value
        auto parentVal = parent_->retrieve(value);
        arr.set(1, ctx->convert<emscripten::val>(parentVal).as<emscripten::val>());

        //Set parameters
        for(int i = 0; i < parameters_.size(); i++) {
            auto paramVal = parameters_[i]->retrieve(value);
            arr.set(i + 2, ctx->convert<emscripten::val>(paramVal).as<emscripten::val>());
        }

        //Invoke function
        auto ret = impl_.call<emscripten::val>("apply", emscripten::val::null(), arr);
        
        return SIPlus::text::make_data(decay(ret));
    }

private:
    std::weak_ptr<SIPlus::SIPlusParserContext> context_;
    std::shared_ptr<SIPlus::text::ValueRetriever> parent_;
    std::vector<std::shared_ptr<SIPlus::text::ValueRetriever>> parameters_;
    emscripten::val impl_;
};

struct JsFunctionImpl : SIPlus::Function {
    JsFunctionImpl(
        std::weak_ptr<SIPlus::SIPlusParserContext> context,
        emscripten::val impl
    ) : context_(context), impl_(impl) {
        assert_typeof("function_impl", impl_, "function");
    }

    std::shared_ptr<SIPlus::text::ValueRetriever> value(
        std::shared_ptr<SIPlus::text::ValueRetriever> parent, 
        std::vector<std::shared_ptr<SIPlus::text::ValueRetriever>> parameters
    ) const override {
        auto ctx = context_.lock();
        return std::make_shared<JsFunctionValueRetriever>(ctx, parent, parameters, impl_);
    }

private:
    std::weak_ptr<SIPlus::SIPlusParserContext> context_;
    emscripten::val impl_;
};

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
        std::shared_ptr<SIPlus::SIPlusParserContext> context,
        std::shared_ptr<SIPlus::text::ValueRetriever> retriever
    ) : retriever_(retriever), context_(context) {}

    emscripten::val
    retrieve(emscripten::val value) {
        auto context = get_context_from_opts(context_, value);
        auto result = retriever_->retrieve(*context);

        if(!result.is<emscripten::val>()) {
            result = context_->convert<emscripten::val>(result);
        }

        return result.as<emscripten::val>();
    }

private:
    std::shared_ptr<SIPlus::text::ValueRetriever> retriever_;
    std::shared_ptr<SIPlus::SIPlusParserContext> context_;
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
        attach_stl(parser_.context().shared_from_this());
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
