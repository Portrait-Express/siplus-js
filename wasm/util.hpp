#include <emscripten/val.h>
#include <string>

void assert_typeof(const std::string& name, const emscripten::val& val, const std::string& type);
