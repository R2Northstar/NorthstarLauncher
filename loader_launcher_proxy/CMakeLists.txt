add_library(loader_launcher_proxy SHARED
            dllmain.cpp
            framework.h
            pch.cpp
            pch.h
            )

add_library(Northstar::loader_launcher_proxy ALIAS loader_launcher_proxy)

target_link_libraries(loader_launcher_proxy PRIVATE
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
                      )

target_precompile_headers(loader_launcher_proxy PRIVATE pch.h)

target_compile_definitions(loader_launcher_proxy PRIVATE
                           UNICODE
                           _UNICODE
                           )

set_target_properties(loader_launcher_proxy PROPERTIES
                      OUTPUT_NAME launcher
                      )

if(MSVC)
	set_target_properties(loader_launcher_proxy PROPERTIES
	                      LINK_FLAGS "/MANIFESTUAC:NO /SUBSYSTEM:WINDOWS"
	                      )
endif()