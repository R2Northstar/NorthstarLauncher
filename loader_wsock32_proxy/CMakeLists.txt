# Add files to the library
add_library(loader_wsock32_proxy SHARED "dllmain.cpp" "loader.cpp")

# Define includes
include_directories("../include/")

# Precompile pch
target_precompile_headers(loader_wsock32_proxy PRIVATE "pch.h")

# Link dynamic libraries
target_link_libraries(loader_wsock32_proxy
	"${CMAKE_CURRENT_SOURCE_DIR}/../include/MinHook.x64.lib"
	"${CMAKE_CURRENT_SOURCE_DIR}/../include/MinHook.a"
	wsock32
	shlwapi
	kernel32
)

set_target_properties(loader_wsock32_proxy PROPERTIES OUTPUT_NAME wsock32)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VS_PLATFORM_TOOLSET v143)
