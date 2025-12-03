set(EM_COMPILE_ARGS 
    -Wno-unused-command-line-argument 
    -DNDEBUG
    -sMALLOC=emmalloc
    -sMODULARIZE 
    -flto
    -fwasm-exceptions
)

add_compile_options(${EM_COMPILE_ARGS})
add_link_options(${EM_COMPILE_ARGS})

add_subdirectory(dependencies)
add_subdirectory(wasm)
