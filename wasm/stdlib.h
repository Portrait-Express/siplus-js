#ifndef INCLUDE_WASM_STDLIB_H_
#define INCLUDE_WASM_STDLIB_H_

#include <emscripten/bind.h>
#include "siplus/context.h"

int attach_stl(std::shared_ptr<SIPlus::SIPlusParserContext> context);

/**
 * @brief Decays an `emscripten::val` into a base type for SIPlus if it is 
 * possible. This means string -> std::string, number -> double, boolean -> 
 * bool and so on
 *
 * @param val The value to decay
 */
SIPlus::text::UnknownDataTypeContainer decay(const emscripten::val& val);

#endif  // INCLUDE_WASM_STDLIB_H_
