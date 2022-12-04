# Load resource files
set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/resources.rc")

# Create exe and set its icon
add_executable (NorthstarLauncher "main.cpp" ${APP_ICON_RESOURCE_WINDOWS})

# Link dynamic libraries
target_link_libraries(NorthstarLauncher PRIVATE
                      shlwapi.lib
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
                      Ws2_32.lib
                      )

target_compile_definitions(NorthstarLauncher PRIVATE
	UNICODE
	_UNICODE
)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:8000000")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_VS_PLATFORM_TOOLSET v143)
