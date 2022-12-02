add_library(loader_wsock32_proxy SHARED "dllmain.cpp" "loader.cpp")

target_precompile_headers(loader_wsock32_proxy PRIVATE "pch.h")

include_directories("../NorthstarDLL/include/")

target_link_libraries(loader_wsock32_proxy "${CMAKE_CURRENT_SOURCE_DIR}/../NorthstarDLL/include/MinHook.x64.lib" "${CMAKE_CURRENT_SOURCE_DIR}/../NorthstarDLL/include/MinHook.a" wsock32 shlwapi kernel32)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET loader_launcher_proxy PROPERTY CXX_STANDARD 20)
endif()
