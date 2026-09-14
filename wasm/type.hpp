#pragma once
#include "siplus/context.hxx"
#ifndef INCLUDE_WASM_LIB_HPP_
#define INCLUDE_WASM_LIB_HPP_

#include <siplus/siplus.hxx>
#include <emscripten/bind.h>
#include <string>

using namespace SIPlus;

int attach_stl(std::shared_ptr<SIPlus::SIPlusParserContext> context);

/**
 * @brief Decays an `emscripten::val` into a base type for SIPlus if it is 
 * possible. This means string -> std::string, number -> double, boolean -> 
 * bool and so on
 *
 * @param val The value to decay
 */
SIPlus::UnknownDataTypeContainer jsToCpp(const emscripten::val& val);

emscripten::val cppToJs(const SIPlus::UnknownDataTypeContainer& val);

template<simple_value_retrievable_type T>
SIPlus::UnknownDataTypeContainer conv(const emscripten::val val) {
    std::string type = val.typeOf().as<std::string>();

    if(type == "string") {
        if constexpr (std::same_as<T, std::string>) {
            return make_data(val.as<std::string>());
        } else {
            return make_data(val.as<std::string>()).convert<T>();
        }
    } else if(type == "number") {
        if constexpr (std::same_as<T, long>) {
            return make_data(val.as<long>());
        } else if constexpr(std::same_as<T, double>) {
            return make_data(val.as<double>());
        } else {
            return SIPlus::make_data(val.as<double>());
        }
    } else if(type == "boolean") {
        return SIPlus::make_data(val.as<bool>());
    } else {
        throw std::runtime_error{"Cannot convert from " + type + " to " + T{}.name()};
    }
}

/**
 * @brief TypeInfo for JS values
 */
struct JSType : SIPlus::TypeInfo {
    using data_type = emscripten::val;
    static const TypeInfoDescriptor *const descriptor;

    JSType();

    bool is_iterable(const UnknownDataTypeContainer& data) const override;

    UnknownDataTypeContainer access(const UnknownDataTypeContainer& data, const std::string& name) const override;

    UnknownDataTypeContainer index(
        std::shared_ptr<SIPlusParserContext> context, 
        UnknownDataTypeContainer &value, 
        UnknownDataTypeContainer &index
    ) const override;

    std::unique_ptr<Iterator> iterate(const UnknownDataTypeContainer& data) const override;
};

namespace SIPlus {
SIPLUS_DEFINE_TYPE_INFO(emscripten::val, JSType);
}



#endif  // INCLUDE_WASM_LIB_HPP_
