set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-command-line-argument -fexceptions --bind -s MODULARIZE -s SINGLE_FILE=1 -s EXPORT_ES6=1")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wno-unused-command-line-argument -fexceptions")

add_subdirectory(dependencies)
add_subdirectory(wasm)
