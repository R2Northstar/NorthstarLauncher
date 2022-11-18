cmake_minimum_required(VERSION 3.15)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
	message(FATAL_ERROR "In-tree builds are not supported. Run CMake from a separate directory: cmake -B build")
endif()

project(r2northstar LANGUAGES CXX ASM)

add_subdirectory(NorthstarLauncher)
add_subdirectory(loader_launcher_proxy)
add_subdirectory(loader_wsock32_proxy)