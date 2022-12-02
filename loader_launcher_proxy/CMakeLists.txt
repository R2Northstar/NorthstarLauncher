add_library(loader_launcher_proxy SHARED "dllmain.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET loader_launcher_proxy PROPERTY CXX_STANDARD 20)
endif()

target_link_libraries(loader_launcher_proxy shlwapi kernel32)
