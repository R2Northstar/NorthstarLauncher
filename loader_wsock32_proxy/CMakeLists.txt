add_library(wsock32 SHARED
    dllmain.cpp
    loader.cpp
    wsock32.s
)

target_include_directories(wsock32 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/)
target_link_libraries(wsock32 LINK_PUBLIC shlwapi msvcr120)
# TODO check for mingw
target_link_libraries(wsock32 LINK_PUBLIC ${CMAKE_SOURCE_DIR}/libMinHook.a)

target_compile_features(wsock32 PUBLIC
	cxx_std_20
)

set_target_properties(wsock32 PROPERTIES PREFIX "")