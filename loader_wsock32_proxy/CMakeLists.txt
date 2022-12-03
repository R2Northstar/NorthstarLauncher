# Add files to the library
add_library(wsock32 SHARED "dllmain.cpp" "loader.cpp" "loader.h")

# Define includes
include_directories("../include/")
include_directories("./")

# Link dynamic libraries
target_link_libraries(wsock32
						"${CMAKE_CURRENT_SOURCE_DIR}/../include/MinHook.a"
						mswsock.lib
						ws2_32.lib
						Shlwapi.lib
						imagehlp.lib
						dbghelp.lib
						kernel32.lib
						user32.lib
						gdi32.lib
						winspool.lib
						comdlg32.lib
						advapi32.lib
						shell32.lib
						ole32.lib
						oleaut32.lib
						uuid.lib
						odbc32.lib
						odbccp32.lib
)
target_compile_definitions(wsock32 PRIVATE
                        UNICODE
                        _UNICODE
						_DEBUG
						LOADERWSOCK32PROXY_EXPORTS
						_WINDOWS
						_USRDLL
)

target_link_options(wsock32 PUBLIC "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/wsock32.def" "/OPT:REF" "/SUBSYSTEM:WINDOWS" "/INCREMENTAL" "/MANIFESTUAC:NO" "/DEBUG" "/NXCOMPAT" "/DYNAMICBASE")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VS_PLATFORM_TOOLSET v143)
