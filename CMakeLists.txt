cmake_minimum_required(VERSION 3.23)

project(Northstar CXX ASM_MASM)

# Language specs
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VS_PLATFORM_TOOLSET v143)

# Targets
add_subdirectory(loader_wsock32_proxy)
add_subdirectory(NorthstarDLL)
add_subdirectory(NorthstarLauncher)
