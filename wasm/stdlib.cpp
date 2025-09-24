#include <emscripten/bind.h>

#include "siplus/context.h"
#include "siplus/stl.h"
#include "siplus/text/accessor.h"
#include "siplus/text/converter.h"
#include "siplus/text/data.h"

#include "stdlib.h"

SIPlus::text::UnknownDataTypeContainer decay(emscripten::val val) {
    if(val.isString()) {
        return SIPlus::text::make_data(val.as<std::string>());
    } else if(val.isNumber()) {
        return SIPlus::text::make_data(val.as<double>());
    } else if(val.isTrue() || val.isFalse()) {
        return SIPlus::text::make_data(val.as<bool>());
    } else if(val.isNull() || val.isUndefined()) {
        return SIPlus::text::UnknownDataTypeContainer{};
    } else {
        return SIPlus::text::make_data(val);
    }
}

struct JsAccessor : SIPlus::text::Accessor {
    SIPlus::text::UnknownDataTypeContainer 
    access(const SIPlus::text::UnknownDataTypeContainer &value, 
           const std::string &name) override {
        emscripten::val result;
        const emscripten::val& jVal = value.as<emscripten::val>();

        auto type = jVal.typeOf().as<std::string>();
        if(type == "object") {
            result = jVal[name];
        } else {
            throw std::runtime_error{"Cannot read properties off of " 
                + name + ". Not an object."};
        }

        return decay(result);
    }

    bool can_access(const SIPlus::text::UnknownDataTypeContainer &value) override {
        return value.is<emscripten::val>();
    }
};

struct JsStringConverter : SIPlus::text::Converter {
    SIPlus::text::UnknownDataTypeContainer 
    convert(SIPlus::text::UnknownDataTypeContainer from, std::type_index to) override {
        auto& val = from.as<emscripten::val>();
        auto string = val.call<emscripten::val>("toString");
        emscripten::val::global("console").call<emscripten::val>("log", string);
        return SIPlus::text::make_data(string.as<std::string>());
    }

    bool can_convert(std::type_index from, std::type_index to) override {
        return from == typeid(emscripten::val) && to == typeid(std::string);
    }
};

int attach_stl(std::shared_ptr<SIPlus::SIPlusParserContext> context) {
    context->emplace_accessor<JsAccessor>();
    context->emplace_converter<JsStringConverter>();

    SIPlus::stl::attach_stl(*context);

    return 0;
}
