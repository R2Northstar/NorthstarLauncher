# Collect all cpp files recursively
file(GLOB_RECURSE sources *.cpp)

# Add files to the library
add_library (NorthstarDLL SHARED ${sources})

# Define includes
include_directories("../include/")
include_directories("./")

# Precompile pch
target_precompile_headers(NorthstarDLL PRIVATE "pch.h")

set(CURL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../include/libcurl)

find_package( CURL REQUIRED )

# Link dynamic libraries
target_link_libraries(NorthstarDLL
	"${CMAKE_CURRENT_SOURCE_DIR}/../include/MinHook.a"
	CURL::libcurl
	dbghelp.lib
    Wldap32.lib
    ws2_32.lib
)

# Add predefined macros
target_compile_definitions(NorthstarDLL PRIVATE
                           UNICODE
                           _UNICODE
                           CURL_STATICLIB
                           )

# Set the project to the right dll name
set_target_properties(NorthstarDLL PROPERTIES OUTPUT_NAME Northstar)

# Add compiler flags
target_compile_options(NorthstarDLL PUBLIC
					-Wno-unused-variable		# We use a lot of unused variables in our decompiles
					-Wno-unused-private-field	# We use a lot of unused private fields in decompiled structs
					-Wno-microsoft-cast			# Pointer-to-func to Pointer-to-obj conversion warning
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VS_PLATFORM_TOOLSET v143)
