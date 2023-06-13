cmake_minimum_required(VERSION 3.12)

project(Northstar CXX ASM_MASM)

# Language specs
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VS_PLATFORM_TOOLSET v143)

# This determines the real binary root directory
set(NS_BINARY_DIR ${CMAKE_SOURCE_DIR}/game)

# Targets
add_subdirectory(loader_wsock32_proxy)
add_subdirectory(NorthstarDLL)
add_subdirectory(NorthstarLauncher)
