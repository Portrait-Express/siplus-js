#pragma once
#ifndef INCLUDE_WASM_VALUE_HPP_
#define INCLUDE_WASM_VALUE_HPP_

#include <siplus/siplus.hxx>
#include <emscripten/bind.h>

using namespace SIPlus;

struct JsFunctionImpl : Function {
    JsFunctionImpl(emscripten::val impl);

    std::shared_ptr<ValueRetriever> value(
        std::shared_ptr<ValueRetriever> parent, 
        std::vector<std::shared_ptr<ValueRetriever>> parameters
    ) const override;

private:
    emscripten::val impl_;
};


#endif  // INCLUDE_WASM_VALUE_HPP_
