#include "siplus/context.h"
#include "siplus/function.h"
#include "siplus/parser.h"
#include "siplus/text/constructor.h"
#include "siplus/text/data.h"

#include <emscripten/bind.h>
#include <stdexcept>
#include <string>

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
        auto result = retriever_->retrieve(decay(value));

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
        std::shared_ptr<SIPlus::text::ValueRetriever> parent,
        std::vector<std::shared_ptr<SIPlus::text::ValueRetriever>> parameters,
        emscripten::val impl
    ) : context_(context), parent_(parent), parameters_(parameters), impl_(impl) {
        assert_typeof("function_impl", impl_, "function");
    }

    SIPlus::text::UnknownDataTypeContainer retrieve(
        const SIPlus::text::UnknownDataTypeContainer &value
    ) const override {
        auto ctx = context_.lock();
        auto arr = emscripten::val::global("Array").new_(parameters_.size() + 2);

        //Base value
        arr.set(0, ctx->convert<emscripten::val>(value).as<emscripten::val>());

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
