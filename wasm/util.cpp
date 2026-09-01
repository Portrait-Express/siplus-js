#include "util.hpp"

#include <siplus/siplus.hxx>

void assert_typeof(const std::string& name, const emscripten::val& val, const std::string& type) {
    if(val.typeOf().as<std::string>() != type) {
        auto msg = SIPlus::util::to_string(
            "Expected ", name, " to be type ", type, 
            " got ", val.typeOf().as<std::string>());

        throw std::runtime_error{msg};
    }
}

