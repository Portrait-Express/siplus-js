set(EM_COMPILE_ARGS 
    -Wno-unused-command-line-argument 
    -DNDEBUG
    -sMALLOC=emmalloc
    -sMODULARIZE 
    "-sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE=['\$getCppExceptionTag', '\$getCppExceptionThrownObjectFromWebAssemblyException']"
    "-sEXPORTED_FUNCTIONS=['getCppExceptionTag', 'getCppExceptionThrownObjectFromWebAssemblyException']"
    -sEXPORT_EXCEPTION_HANDLING_HELPERS
    -flto
    -fwasm-exceptions
    --bind 
)

add_compile_options(${EM_COMPILE_ARGS})
add_link_options(${EM_COMPILE_ARGS})

add_subdirectory(dependencies)
add_subdirectory(wasm)
