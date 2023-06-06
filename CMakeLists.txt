cmake_minimum_required(VERSION 3.23)

project(Northstar CXX)

# Language specs
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VS_PLATFORM_TOOLSET v143)

# Build directory
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/game)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Targets
#add_subdirectory(loader_launcher_proxy)
#add_subdirectory(loader_wsock32_proxy)
#add_subdirectory(NorthstarDLL)
add_subdirectory(NorthstarLauncher)