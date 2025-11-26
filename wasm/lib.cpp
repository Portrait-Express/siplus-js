#include "siplus/context.h"
#include "siplus/function.h"
#include "siplus/parser.h"
#include "siplus/text/constructor.h"
#include "siplus/text/data.h"

#include <emscripten/bind.h>
#include <stdexcept>
#include <string>

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

class EM_TextConstructor {
public:
    EM_TextConstructor(SIPlus::text::TextConstructor constructor) : constructor_(constructor) {}

    std::string
    construct(emscripten::val value) {
        return constructor_.construct_with(SIPlus::text::make_data(value));
    }

private:
    SIPlus::text::TextConstructor constructor_;
};

class EM_ValueRetriever {
public:
    EM_ValueRetriever(
        std::shared_ptr<SIPlus::SIPlusParserContext> context,
        std::shared_ptr<SIPlus::text::ValueRetriever> retriever
    ) : retriever_(retriever), context_(context) {}

    emscripten::val
    retrieve(emscripten::val value) {
        SIPlus::text::UnknownDataTypeContainer value_container = SIPlus::text::make_data(value);

        SIPlus::text::UnknownDataTypeContainer result = retriever_->retrieve(value_container);

        if(!result.is<emscripten::val>()) {
            result = context_->convert<emscripten::val>(result);
        }

        return result.as<emscripten::val>();
    }

private:
    std::shared_ptr<SIPlus::text::ValueRetriever> retriever_;
    std::shared_ptr<SIPlus::SIPlusParserContext> context_;
};

struct JsFunctionValueRetriever : SIPlus::text::ValueRetriever {
    JsFunctionValueRetriever(
        std::weak_ptr<SIPlus::SIPlusParserContext> context,
        emscripten::val impl
    ) : context_(context), impl_(impl) { }

    SIPlus::text::UnknownDataTypeContainer retrieve(
        const SIPlus::text::UnknownDataTypeContainer &value) const override {
        auto ctx = context_.lock();
        auto val = ctx->convert<emscripten::val>(value).as<emscripten::val>();
        auto ret = impl_(val);
        return SIPlus::text::make_data(ret);
    }

private:
    std::weak_ptr<SIPlus::SIPlusParserContext> context_;
    emscripten::val impl_;
};

struct JsFunctionImpl : SIPlus::Function {
    JsFunctionImpl(
        emscripten::val impl,
        std::weak_ptr<SIPlus::SIPlusParserContext> context
    ) : impl_(impl), context_(context) {}

    std::shared_ptr<SIPlus::text::ValueRetriever> value(
        std::shared_ptr<SIPlus::text::ValueRetriever> parent, 
        std::vector<std::shared_ptr<SIPlus::text::ValueRetriever>> parameters
    ) const override {
        assert_typeof("function_impl", impl_, "function");

        auto ctx = context_.lock();

        emscripten::val parent_val = emscripten::val::null();
        auto arr = emscripten::val::global("Array").new_(parameters.size());

        if(parent) {
            parent_val = emscripten::val{EM_ValueRetriever{ctx, parent}};
        }
        
        for(int i = 0; i < parameters.size(); i++) {
            arr.set(i, EM_ValueRetriever{ctx, parameters[i]});
        }
            
        auto ret = impl_(parent_val, arr);

        return std::make_shared<JsFunctionValueRetriever>(ctx, ret);
    }

private:
    std::weak_ptr<SIPlus::SIPlusParserContext> context_;
    emscripten::val impl_;
};

class EM_SIParserContext {
public:
    EM_SIParserContext(std::shared_ptr<SIPlus::SIPlusParserContext> context)
        : context_(context) {}

    void emplace_function(emscripten::val name, emscripten::val impl) {
        assert_typeof("name", name, "string");
        assert_typeof("impl", impl, "function");

        context_->emplace_function<JsFunctionImpl>(name.as<std::string>(), impl, context_);
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
    get_expression(emscripten::val text) {
        if(!text.isString()) {
            throw std::runtime_error{"expected first argument to be string. Got " + 
                text.typeOf().as<std::string>()};
        }

        return EM_ValueRetriever{
            parser_.context().shared_from_this(),
            parser_.get_expression(text.as<std::string>())
        };
    }

    EM_TextConstructor
    get_interpolated(emscripten::val text) {
        if(!text.isString()) {
            throw std::runtime_error{"expected first argument to be string. Got " + 
                text.typeOf().as<std::string>()};
        }

        return parser_.get_interpolation(text.as<std::string>());
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
    emscripten::function("getExceptionMessage", &getExceptionMessage);

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
