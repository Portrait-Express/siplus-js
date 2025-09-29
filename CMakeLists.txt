set(EM_COMPILE_ARGS 
    -Wno-unused-command-line-argument 
    -fexceptions 
    --bind 
    -sMODULARIZE 
    -sSINGLE_FILE=1 
)

add_compile_options(${EM_COMPILE_ARGS})
add_link_options(${EM_COMPILE_ARGS})

add_subdirectory(dependencies)
add_subdirectory(wasm)
