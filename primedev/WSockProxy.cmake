# loader_wsock32_proxy

find_package(minhook REQUIRED)

add_library(
    loader_wsock32_proxy SHARED
    "wsockproxy/dllmain.cpp"
    "wsockproxy/loader.cpp"
    "wsockproxy/loader.h"
    "wsockproxy/wsock32.asm"
    "wsockproxy/wsock32.def"
    )

target_link_libraries(
    loader_wsock32_proxy
    PRIVATE minhook
            mswsock.lib
            ws2_32.lib
            ShLwApi.lib
            imagehlp.lib
            dbghelp.lib
            kernel32.lib
            user32.lib
            gdi32.lib
            winspool.lib
            comdlg32.lib
            advapi32.lib
            shell32.lib
            ole32.lib
            oleaut32.lib
            uuid.lib
            odbc32.lib
            odbccp32.lib
    )

target_precompile_headers(
    loader_wsock32_proxy
    PRIVATE
    wsockproxy/pch.h
    )

target_compile_definitions(loader_wsock32_proxy PRIVATE UNICODE _UNICODE)

set_target_properties(
    loader_wsock32_proxy
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${NS_BINARY_DIR}/bin/x64_retail
               OUTPUT_NAME wsock32
               LINK_FLAGS "/MANIFEST:NO /DEBUG"
    )

# COMPILE_LANGUAGE is used here because of the ASM file
if(MSVC)
    target_compile_options(loader_wsock32_proxy PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/W4 /WX>)
else()
    target_compile_options(
        loader_wsock32_proxy
        PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-Wall
                -Wextra
                -Wpedantic
                -Werror>
        )
endif()
