#ifndef INCLUDE_WASM_STDLIB_H_
#define INCLUDE_WASM_STDLIB_H_

#include <emscripten/bind.h>
#include "siplus/context.hxx"

int attach_stl(std::shared_ptr<SIPlus::SIPlusParserContext> context);

/**
 * @brief Decays an `emscripten::val` into a base type for SIPlus if it is 
 * possible. This means string -> std::string, number -> double, boolean -> 
 * bool and so on
 *
 * @param val The value to decay
 */
SIPlus::UnknownDataTypeContainer decay(const emscripten::val& val);

struct JSType : SIPlus::TypeInfo {
    using data_type = emscripten::val;

    std::string name() const override;
    bool is_iterable(const SIPlus::UnknownDataTypeContainer& data) const override;
    SIPlus::UnknownDataTypeContainer access(const SIPlus::UnknownDataTypeContainer& data, const std::string& name) const override;
    std::unique_ptr<SIPlus::text::Iterator> iterate(const SIPlus::UnknownDataTypeContainer& data) const override;
};

namespace SIPlus {
SIPLUS_DEFINE_TYPE_INFO(emscripten::val, JSType);
}

#endif  // INCLUDE_WASM_STDLIB_H_
