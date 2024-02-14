# NorthstarLauncher

add_executable(NorthstarLauncher "primelauncher/main.cpp" "primelauncher/resources.rc")

target_compile_definitions(NorthstarLauncher PRIVATE UNICODE _UNICODE)

target_link_libraries(
    NorthstarLauncher
    PRIVATE shlwapi.lib
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
            WS2_32.lib
    )

set_target_properties(
    NorthstarLauncher PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${NS_BINARY_DIR} LINK_FLAGS
                                                                           "/MANIFEST:NO /DEBUG /STACK:8000000"
    )

if(MSVC)
    target_compile_options(NorthstarLauncher PRIVATE /W4 /WX)
else()
    target_compile_options(
        NorthstarLauncher
        PRIVATE -Wall
                -Wextra
                -Wpedantic
                -Werror
        )
endif()
