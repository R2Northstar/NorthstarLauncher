cmake_minimum_required(VERSION 3.23)

project(NorthstarLauncher CXX)

if(MSVC)
	enable_language(ASM_MASM)
	add_compile_options("/sdl" "/EHsc")
else()
	enable_language(ASM)
	set(CMAKE_ASM_FLAGS "${CFLAGS} -x assembler-with-cpp -masm=intel")
	add_compile_options("-Wa,-mbig-obj")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VS_PLATFORM_TOOLSET v143)
# The VS build sends everything to one directory, so do that for now, even though we probably only need the dlls and exe
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(loader_launcher_proxy)
add_subdirectory(loader_wsock32_proxy)
add_subdirectory(NorthstarDLL)
add_subdirectory(NorthstarLauncher)
