# NorthstarLauncher

find_package(minhook REQUIRED)

add_library(NorthstarLoader OBJECT "primelauncher/loader.cpp" "primelauncher/loader.h")

target_compile_definitions(NorthstarLoader PUBLIC UNICODE _UNICODE)

target_link_libraries(NorthstarLoader PUBLIC minhook shlwapi.lib)

add_executable(NorthstarLauncher "primelauncher/main.cpp" "primelauncher/resources.rc")

target_link_libraries(NorthstarLauncher PRIVATE NorthstarLoader)

set_target_properties(
    NorthstarLauncher PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${NS_BINARY_DIR} LINK_FLAGS
                                                                           "/MANIFEST:NO /DEBUG /STACK:8000000"
    )

add_library(
    NorthstarWsockProxy SHARED
    "primelauncher/dllmain.cpp"
    "primelauncher/wsock32.asm"
    "primelauncher/wsock32.def"
    )

target_link_libraries(
    NorthstarWsockProxy
    PRIVATE NorthstarLoader
            minhook
            mswsock.lib
            ws2_32.lib
    )

set_target_properties(
    NorthstarWsockProxy
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${NS_BINARY_DIR}/bin/x64_retail
               OUTPUT_NAME wsock32
               LINK_FLAGS "/MANIFEST:NO /DEBUG"
    )
