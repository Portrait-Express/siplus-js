set(EM_COMPILE_ARGS 
    -Wno-unused-command-line-argument 
    -fexceptions 
    --bind 
    -sASSERTIONS=0
    -Os
    -DNDEBUG
    -flto
    -sMALLOC=emmalloc
    -sMODULARIZE 
)

add_compile_options(${EM_COMPILE_ARGS})
add_link_options(${EM_COMPILE_ARGS})

add_subdirectory(dependencies)
add_subdirectory(wasm)
