include_directories("../include/")
include_directories("./")

file(GLOB_RECURSE sources *.cpp)

add_library (NorthstarDLL SHARED ${sources} "squirrel/squirrel.cpp")

target_precompile_headers(NorthstarDLL PRIVATE "pch.h")

target_link_libraries(NorthstarDLL PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../include/MinHook.x64.lib" "${CMAKE_CURRENT_SOURCE_DIR}/../include/MinHook.a" "${CMAKE_CURRENT_SOURCE_DIR}/../include/libcurl/lib/libcurl.dll.a" dbghelp)


if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET NorthstarDLL PROPERTY CXX_STANDARD 20)
endif()
