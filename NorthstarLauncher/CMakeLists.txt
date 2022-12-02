# Load resource files
set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/resources.rc")

# Create exe and set its icon
add_executable (NorthstarLauncher "main.cpp" ${APP_ICON_RESOURCE_WINDOWS})

# Link dynamic libraries
target_link_libraries(NorthstarLauncher wsock32 ws2_32 shlwapi)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VS_PLATFORM_TOOLSET v143)
