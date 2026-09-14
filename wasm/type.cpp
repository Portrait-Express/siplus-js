#include "type.hpp"
#include "siplus/context.hxx"

#include <stdexcept>

using namespace SIPlus;

UnknownDataTypeContainer jsToCpp(const emscripten::val& val) {
    if(val.isString()) {
        return make_data(val.as<std::string>());
    } else if(val.isNumber()) {
        return make_data(val.as<double>());
    } else if(val.isNull() || val.isUndefined()) {
        return UnknownDataTypeContainer{};
    } else if(val.isTrue() || val.isFalse()) {
        return make_data(val.as<bool>());
    } else {
        return make_data(val);
    }
}

emscripten::val cppToJs(const UnknownDataTypeContainer& val) {
    if(val.is<JSType>()) {
        return val.as<JSType>();
    } else if(val.is<types::IntegerType>()) {
        return emscripten::val{(double)val.as<types::IntegerType>()};
    } else if(val.is<types::FloatType>()) {
        return emscripten::val{val.as<types::FloatType>()};
    } else if(val.is<types::StringType>()) {
        return emscripten::val{val.as<types::StringType>()};
    } else if(val.is<types::BoolType>()) {
        return emscripten::val{val.as<types::BoolType>()};
    } else if(val.is<types::NullType>()) {
        return emscripten::val::null();
    }

    if(val.is_iterable()) {
        emscripten::val arr = emscripten::val::global("Array").new_();
        auto iterator = val.iterate();

        while(iterator->more()) {
            iterator->next();

            arr.call<void>("push", cppToJs(iterator->current()));
        }

        return arr;
    }

    throw std::runtime_error{util::to_string("Cannot convert from '", val.type->name(), "' to a js type.")};
}



namespace {

struct JSIterator : Iterator {
    JSIterator(emscripten::val iterator);

    void next() override;
    bool more() override;
    UnknownDataTypeContainer current() override;

private:
    emscripten::val iterator_;
    emscripten::val last_;
    emscripten::val next_;
};

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

UnknownDataTypeContainer JSIterator::current() {
    return jsToCpp(last_["value"]);
}



struct JsStringConverter : UnaryOperatorTypeExtensionImpl {
    JsStringConverter() : UnaryOperatorTypeExtensionImpl(make_conversion_descriptor<types::StringType>()) {}

    UnknownDataTypeContainer invoke(const UnknownDataTypeContainer &thisValue) const override {
        auto text = emscripten::val::global("String")(thisValue.as<JSType>()).as<std::string>();
        return make_data(text);
    }
};



} /* namespace anonymous */



const TypeInfoDescriptor *const JSType::descriptor = new TypeInfoDescriptor( "[ Object ]");
JSType::JSType() : TypeInfo(descriptor) {
    add_extension(new JsStringConverter());
}

bool JSType::is_iterable(const UnknownDataTypeContainer& data) const {
    emscripten::val& val = data.as<JSType>();
    auto iterator = emscripten::val::global("Symbol")["iterator"];

    if(val[iterator].typeOf().as<std::string>() != "function") {
        return false;
    } else {
        return true;
    }
}

UnknownDataTypeContainer JSType::access(const UnknownDataTypeContainer& data, const std::string& name) const {
    emscripten::val result;
    const emscripten::val& jVal = data.as<JSType>();

    auto type = jVal.typeOf().as<std::string>();
    if(type == "object") {
        result = jVal[name];
    } else {
        throw std::runtime_error{util::to_string(
            "Cannot access property '", name, "' on value of type ", type
        )};
    }

    return jsToCpp(result);
}

UnknownDataTypeContainer JSType::index(
    std::shared_ptr<SIPlusParserContext> context, 
    UnknownDataTypeContainer &value, 
    UnknownDataTypeContainer &index
) const {
    emscripten::val jsIndex = cppToJs(index);

    return jsToCpp(value.as<JSType>()[jsIndex]);
}

std::unique_ptr<Iterator> JSType::iterate(const UnknownDataTypeContainer& data) const {
    const auto& jVal = data.as<JSType>();
    auto symbol = emscripten::val::global("Symbol")["iterator"];
    auto iterator = jVal[symbol].call<emscripten::val>("apply", jVal);

    return std::make_unique<JSIterator>(iterator);
}
