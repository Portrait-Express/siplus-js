#pragma once
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

/**
 * @brief TypeInfo for JS values
 */
struct JSType : SIPlus::TypeInfo {
    using data_type = emscripten::val;

    std::string name() const override;

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

struct JSIterator : Iterator {
    JSIterator(emscripten::val iterator);

    void next() override;
    bool more() override;
    SIPlus::UnknownDataTypeContainer current() override;

private:
    emscripten::val iterator_;
    emscripten::val last_;
    emscripten::val next_;
};

/**
 * Converter to convert a native array to a JS array.
 */
struct JsArrayConverter : Converter {
    JsArrayConverter(std::shared_ptr<SIPlus::SIPlusParserContext> context);

    bool can_convert(const SIPlus::TypeInfo& from, const SIPlus::TypeInfo& to) const override;

    SIPlus::UnknownDataTypeContainer 
    convert(const SIPlus::UnknownDataTypeContainer& from, const SIPlus::TypeInfo& to) const override;

private:
    std::shared_ptr<SIPlus::SIPlusParserContext> context_;
};

/**
 * Converter to convert from a standard data object to a JS type.
 */
struct ToJsPrimitiveConverter : Converter {
    bool can_convert(const SIPlus::TypeInfo& from, const SIPlus::TypeInfo& to) const override;

    SIPlus::UnknownDataTypeContainer 
    convert(const SIPlus::UnknownDataTypeContainer& from, const SIPlus::TypeInfo& to) const override;
};

/**
 * Converter to convert from a JSType data object back to the standard data types.
 */
struct FromJsPrimitiveConverter : Converter {
    FromJsPrimitiveConverter(std::weak_ptr<SIPlus::SIPlusParserContext> ctx);

    SIPlus::UnknownDataTypeContainer 
    convert(const SIPlus::UnknownDataTypeContainer& from, const SIPlus::TypeInfo& to) const override;
    
    bool can_convert(const SIPlus::TypeInfo& from, const SIPlus::TypeInfo& to) const override;

private:
    std::weak_ptr<SIPlus::SIPlusParserContext> ctx_;
};



#endif  // INCLUDE_WASM_LIB_HPP_
