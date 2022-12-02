# Create the dll
add_library(loader_launcher_proxy SHARED "dllmain.cpp")

# Link dynamic libraries
target_link_libraries(loader_launcher_proxy shlwapi kernel32)

set_target_properties(loader_launcher_proxy PROPERTIES OUTPUT_NAME launcher)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VS_PLATFORM_TOOLSET v143)
