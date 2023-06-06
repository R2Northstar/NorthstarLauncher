# NorthstarLauncher

# Needed as we launch titanfall inside ourselves
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /STACK:8000000")

add_executable(NorthstarLauncher "main.cpp" resources.rc)

target_compile_definitions(NorthstarLauncher PRIVATE
						   UNICODE
						   _UNICODE)

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
