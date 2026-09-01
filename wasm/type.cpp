#include "type.hpp"
#include <stdexcept>

SIPlus::UnknownDataTypeContainer jsToCpp(const emscripten::val& val) {
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

emscripten::val cppToJs(const SIPlus::UnknownDataTypeContainer& val) {
    if(val.is<types::IntegerType>()) {
        return emscripten::val{val.as<types::IntegerType>()};
    } else if(val.is<types::FloatType>()) {
        return emscripten::val{val.as<types::FloatType>()};
    } else if(val.is<types::StringType>()) {
        return emscripten::val{val.as<types::StringType>()};
    } else if(val.is<types::BoolType>()) {
        return emscripten::val{val.as<types::BoolType>()};
    } else if(val.is<types::NullType>()) {
        return emscripten::val::null();
    } else {
        throw std::runtime_error{util::to_string("Cannot convert from '", val.type->name(), "' to a js type.")};
    }
}

JSIterator::JSIterator(emscripten::val iterator) : iterator_(iterator) {
    next_ = iterator_.call<emscripten::val>("next");
}

void JSIterator::next() {
    last_ = next_;
    next_ = iterator_.call<emscripten::val>("next");
}

bool JSIterator::more() {
    auto done = next_["done"];
    return !done;
}

SIPlus::UnknownDataTypeContainer JSIterator::current() {
    return jsToCpp(last_["value"]);
}



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

    return jsToCpp(result);
}

UnknownDataTypeContainer JSType::index(
    std::shared_ptr<SIPlusParserContext> context, 
    UnknownDataTypeContainer &value, 
    UnknownDataTypeContainer &index
) const {
    emscripten::val jsIndex;

    if(context->can_convert<JSType>(*index.type)) {
        jsIndex = context->convert<JSType>(index).as<JSType>();
    } else {
        jsIndex = cppToJs(index);
    }

    return jsToCpp(value.as<JSType>()[jsIndex]);
}

std::unique_ptr<Iterator> JSType::iterate(const SIPlus::UnknownDataTypeContainer& data) const {
    const auto& jVal = data.as<JSType>();
    auto symbol = emscripten::val::global("Symbol")["iterator"];
    auto iterator = jVal[symbol].call<emscripten::val>("apply", jVal);

    return std::make_unique<JSIterator>(iterator);
}



JsArrayConverter::JsArrayConverter(std::shared_ptr<SIPlus::SIPlusParserContext> context) : context_(context) {}

bool JsArrayConverter::can_convert(const SIPlus::TypeInfo& from, const SIPlus::TypeInfo& to) const {
    return from.is<SIPlus::types::ArrayType>() && to.is<JSType>();
}

SIPlus::UnknownDataTypeContainer JsArrayConverter::convert(const SIPlus::UnknownDataTypeContainer& from, const SIPlus::TypeInfo& to) const {
    emscripten::val arr = emscripten::val::global("Array").new_();

    for(auto value : from.as<SIPlus::types::ArrayType>()) {
        if(!value.is<JSType>()) {
            value = context_->convert<JSType>(value);
        }

        arr.call<void>("push", value.as<JSType>());
    }

    return SIPlus::make_data(arr);
}



SIPlus::UnknownDataTypeContainer 
ToJsPrimitiveConverter::convert(const SIPlus::UnknownDataTypeContainer& from, const SIPlus::TypeInfo& to) const {
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

bool ToJsPrimitiveConverter::can_convert(const SIPlus::TypeInfo& from, const SIPlus::TypeInfo& to) const {
    return to.is<JSType>() && (
        from.is<SIPlus::types::NullType>() || 
        from.is<SIPlus::types::StringType>() || 
        from.is<SIPlus::types::IntegerType>() || 
        from.is<SIPlus::types::FloatType>() || 
        from.is<SIPlus::types::BoolType>()
    );
}



FromJsPrimitiveConverter::FromJsPrimitiveConverter(std::weak_ptr<SIPlus::SIPlusParserContext> ctx) : ctx_(ctx) { }

SIPlus::UnknownDataTypeContainer 
FromJsPrimitiveConverter::convert(const SIPlus::UnknownDataTypeContainer& from, const SIPlus::TypeInfo& to) const {
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
    
bool FromJsPrimitiveConverter::can_convert(const SIPlus::TypeInfo& from, const SIPlus::TypeInfo& to) const {
    return from.is<JSType>() && (
        to.is<SIPlus::types::NullType>() || 
        to.is<SIPlus::types::IntegerType>() || 
        to.is<SIPlus::types::FloatType>() || 
        to.is<SIPlus::types::BoolType>() || 
        to.is<SIPlus::types::StringType>()
    );
}
