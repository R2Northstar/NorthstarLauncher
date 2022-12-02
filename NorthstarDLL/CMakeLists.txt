include_directories("../include/")
include_directories("./")

file(GLOB_RECURSE sources *.cpp)

add_library (NorthstarDLL SHARED ${sources} "squirrel/squirrel.cpp")

target_precompile_headers(NorthstarDLL PRIVATE "pch.h")

target_link_libraries(NorthstarDLL PRIVATE
	"${CMAKE_CURRENT_SOURCE_DIR}/../include/MinHook.x64.lib"
	"${CMAKE_CURRENT_SOURCE_DIR}/../include/MinHook.a"
	"${CMAKE_CURRENT_SOURCE_DIR}/../include/libcurl/lib/libcurl.dll.a"
	dbghelp
)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VS_PLATFORM_TOOLSET v143)
