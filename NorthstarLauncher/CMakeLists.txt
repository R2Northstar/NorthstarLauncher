add_executable(NorthstarLauncher
               main.cpp
               resource1.h
               resources.rc
               )

add_executable(Northstar::Launcher ALIAS NorthstarLauncher)

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