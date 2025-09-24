#include "siplus/context.h"
#include "siplus/parser.h"
#include "siplus/text/constructor.h"
#include "siplus/text/data.h"
#include "siplus/util.h"

#include <emscripten/bind.h>

#include "stdlib.h"

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
    EM_ValueRetriever(std::shared_ptr<SIPlus::text::ValueRetriever> retriever) : 
        retriever_(retriever) {}

    emscripten::val
    retrieve(emscripten::val value) {
        SIPlus::text::UnknownDataTypeContainer value_container = SIPlus::text::make_data(value);

        SIPlus::text::UnknownDataTypeContainer result = retriever_->retrieve(value_container);

        if(!result.is<emscripten::val>()) {
            throw std::runtime_error{"WASM: Unknown result type from ValueRetriever. "
                "Expected emscripten::val, got " + get_type_name(result.type)};
        }

        return result.as<emscripten::val>();
    }

private:
    std::shared_ptr<SIPlus::text::ValueRetriever> retriever_;
};

class EM_SIParserContext {
public:
    EM_SIParserContext(std::shared_ptr<SIPlus::SIPlusParserContext> context)
        : context_(context) {}

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

        return parser_.get_expression(text.as<std::string>());
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
        .function("parse_expression", &EM_SIParser::get_expression);

    auto context_class = emscripten::class_<EM_SIParserContext>("ParserContext");

    auto retriever_class = emscripten::class_<EM_ValueRetriever>("ValueRetriever")
        .function("retrieve", &EM_ValueRetriever::retrieve);

    auto constructor_class = emscripten::class_<EM_TextConstructor>("TextConstructor")
        .function("construct", &EM_TextConstructor::construct);
}
