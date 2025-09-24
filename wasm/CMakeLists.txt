cmake_minimum_required(VERSION 3.20)

project(siplus_js)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions --bind -s MODULARIZE -s SINGLE_FILE=1 -s EXPORT_ES6=1")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fexceptions")

set(SIPLUS_JS_SOURCES lib.cpp stdlib.cpp)

add_executable(siplus_js ${SIPLUS_JS_SOURCES})
set(EMSCRIPTEN_LINK_FLAGS "-s WASM=1 -s INVOKE_RUN=0")

# This essentially duplicates the flags in em++ to the COMPILE_FLAGS variable
# This is to ensure the flags are included in the compile_commands.json file
execute_process(COMMAND bash -c "echo -n \"\$(em++ --cflags)\"" OUTPUT_VARIABLE EM_CFLAGS)
set_target_properties(siplus_js PROPERTIES COMPILE_FLAGS "${EM_CFLAGS}")

target_link_libraries(siplus_js siplus)
