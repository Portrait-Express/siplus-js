#include <emscripten/bind.h>
#include <iostream>

#include "siplus/context.hxx"
#include "siplus/stl.hxx"
#include "siplus/text/converter.hxx"
#include "siplus/text/range_iterator.hxx"
#include "siplus/data.hxx"
#include "siplus/text/iterator.hxx"
#include "siplus/types/array.hxx"
#include "siplus/types/bool.hxx"
#include "siplus/types/float.hxx"
#include "siplus/types/integer.hxx"
#include "siplus/types/string.hxx"
#include "siplus/util.hxx"

#include "stdlib.h"

SIPlus::UnknownDataTypeContainer decay(const emscripten::val& val) {
    if(val.isString()) {
        return SIPlus::make_data(val.as<std::string>());
    } else if(val.isNumber()) {
        return SIPlus::make_data(val.as<double>());
    } else if(val.isNull()) {
        return SIPlus::UnknownDataTypeContainer{};
    } else if(val.isTrue() || val.isFalse()) {
        return SIPlus::make_data(val.as<bool>());
    } else {
        return SIPlus::make_data(val);
    }
}

struct JSIterator : SIPlus::text::Iterator {
    JSIterator (emscripten::val iterator) : iterator_(iterator) {
        next_ = iterator_.call<emscripten::val>("next");
    }

    void next() override {
        last_ = next_;
        next_ = iterator_.call<emscripten::val>("next");
    }

    bool more() override {
        auto done = next_["done"];
        return !done;
    }

    SIPlus::UnknownDataTypeContainer current() override {
        return decay(last_["value"]);
    }

private:
    emscripten::val iterator_;
    emscripten::val last_;
    emscripten::val next_;
};

std::string JSType::name() const {
    return "[ Object ]";
}

bool JSType::is_iterable(const SIPlus::UnknownDataTypeContainer& data) const {
    emscripten::val& val = data.as<JSType>();
    auto iterator = emscripten::val::global("Symbol")["iterator"];

    if(val[iterator].typeOf().as<std::string>() != "function") {
        return false;
    } else {
        return true;
    }
}

SIPlus::UnknownDataTypeContainer JSType::access(const SIPlus::UnknownDataTypeContainer& data, const std::string& name) const {
    emscripten::val result;
    const emscripten::val& jVal = data.as<JSType>();

    auto type = jVal.typeOf().as<std::string>();
    if(type == "object") {
        result = jVal[name];
    } else {
        throw std::runtime_error{SIPlus::util::to_string(
            "Cannot access property ", name, " on value of type ", type
        )};
    }

    return decay(result);
}

std::unique_ptr<SIPlus::text::Iterator> JSType::iterate(const SIPlus::UnknownDataTypeContainer& data) const {
    const auto& jVal = data.as<JSType>();
    auto symbol = emscripten::val::global("Symbol")["iterator"];
    auto iterator = jVal[symbol].call<emscripten::val>("apply", jVal);

    return std::make_unique<JSIterator>(iterator);
}


struct JsArrayConverter : SIPlus::text::Converter {
    JsArrayConverter(std::shared_ptr<SIPlus::SIPlusParserContext> context) 
        : context_(context) {}

    bool can_convert(const SIPlus::TypeInfo& from, const SIPlus::TypeInfo& to) const override {
        return from.is<SIPlus::types::ArrayType>() && to.is<JSType>();
    }

    SIPlus::UnknownDataTypeContainer 
    convert(const SIPlus::UnknownDataTypeContainer& from, const SIPlus::TypeInfo& to) const override {
        emscripten::val arr = emscripten::val::global("Array").new_();

        for(auto value : from.as<SIPlus::types::ArrayType>()) {
            if(!value.is<JSType>()) {
                value = context_->convert<JSType>(value);
            }

            arr.call<void>("push", value.as<JSType>());
        }

        return SIPlus::make_data(arr);
    }

private:
    std::shared_ptr<SIPlus::SIPlusParserContext> context_;
};

struct ToJsPrimitiveConverter : SIPlus::text::Converter {
    SIPlus::UnknownDataTypeContainer 
    convert(const SIPlus::UnknownDataTypeContainer& from, const SIPlus::TypeInfo& to) const override {
        if(from.is<SIPlus::types::StringType>()) {
            return SIPlus::make_data(emscripten::val{from.as<SIPlus::types::StringType>()});

        } else if(from.is<SIPlus::types::IntegerType>()) {
            return SIPlus::make_data(emscripten::val{static_cast<double>(from.as<SIPlus::types::IntegerType>())});

        } else if(from.is<SIPlus::types::FloatType>()) {
            return SIPlus::make_data(emscripten::val{from.as<SIPlus::types::FloatType>()});

        } else if(from.is<SIPlus::types::BoolType>()) {
            return SIPlus::make_data(emscripten::val{from.as<SIPlus::types::BoolType>()});

        } else if(from.is<SIPlus::types::NullType>()) {
            return SIPlus::make_data(emscripten::val::null());

        } else {
            throw std::runtime_error{"Cannot convert from " + from.type->name() + " to emscripten::val"};
        }
    }
    
    bool can_convert(const SIPlus::TypeInfo& from, const SIPlus::TypeInfo& to) const override {
        return to.is<JSType>() && (
            from.is<SIPlus::types::NullType>() || 
            from.is<SIPlus::types::StringType>() || 
            from.is<SIPlus::types::IntegerType>() || 
            from.is<SIPlus::types::FloatType>() || 
            from.is<SIPlus::types::BoolType>()
        );
    }
};

template<typename T> requires std::is_base_of_v<SIPlus::TypeInfo, T>
SIPlus::UnknownDataTypeContainer conv(
    const SIPlus::SIPlusParserContext& ctx,
    const emscripten::val val
) {
    std::string type = val.typeOf().as<std::string>();

    if(type == "string") {
        if constexpr (std::same_as<T, std::string>) {
            return SIPlus::make_data(val.as<std::string>());
        } else {
            return ctx.convert<T>(SIPlus::make_data(val.as<std::string>()));
        }
    } else if(type == "number") {
        if constexpr (std::same_as<T, long>) {
            return SIPlus::make_data(val.as<long>());
        } else if constexpr(std::same_as<T, double>) {
            return SIPlus::make_data(val.as<double>());
        } else {
            return ctx.convert<T>(SIPlus::make_data(val.as<double>()));
        }
    } else if(type == "boolean") {
        return ctx.convert<T>(SIPlus::make_data(val.as<bool>()));
    } else {
        throw std::runtime_error{"Cannot convert from " + type + " to " + T{}.name()};
    }
}

struct FromJsPrimitiveConverter : SIPlus::text::Converter {
    FromJsPrimitiveConverter(
        std::weak_ptr<SIPlus::SIPlusParserContext> ctx
    ) : ctx_(ctx) { }

    SIPlus::UnknownDataTypeContainer 
    convert(const SIPlus::UnknownDataTypeContainer& from, const SIPlus::TypeInfo& to) const override {
        auto ctx = ctx_.lock();
        auto& val = from.as<JSType>();

        if(to.is<SIPlus::types::StringType>()) {
            auto string = emscripten::val::global("String")
                .call<emscripten::val>("apply", emscripten::val::null(), val);

            return SIPlus::make_data(string.as<std::string>());

        } else if(to.is<SIPlus::types::IntegerType>()) {
            return conv<SIPlus::types::IntegerType>(*ctx, val);

        } else if(to.is<SIPlus::types::FloatType>()) {
            return conv<SIPlus::types::FloatType>(*ctx, val);

        } else if(to.is<SIPlus::types::BoolType>()) {
            return conv<SIPlus::types::BoolType>(*ctx, val);

        } else if(to.is<SIPlus::types::IntegerType>()) {
            return conv<SIPlus::types::NullType>(*ctx, val);

        } else {
            throw std::runtime_error{"Cannot convert from " + from.type->name() + " to " + to.name()};
        }
    }
    
    bool can_convert(const SIPlus::TypeInfo& from, const SIPlus::TypeInfo& to) const override {
        return from.is<JSType>() && (
            to.is<SIPlus::types::NullType>() || 
            to.is<SIPlus::types::IntegerType>() || 
            to.is<SIPlus::types::FloatType>() || 
            to.is<SIPlus::types::BoolType>() || 
            to.is<SIPlus::types::StringType>()
        );
    }

private:
    std::weak_ptr<SIPlus::SIPlusParserContext> ctx_;
};

int attach_stl(std::shared_ptr<SIPlus::SIPlusParserContext> context) {
    context->emplace_converter<JsArrayConverter>(context);
    context->emplace_converter<ToJsPrimitiveConverter>();
    context->emplace_converter<FromJsPrimitiveConverter>(context);

    SIPlus::stl::attach_stl(*context);

    return 0;
}
