add_library(loader_wsock32_proxy SHARED
            dllmain.cpp
            loader.cpp
            loader.h
            pch.cpp
            pch.h
            wsock32.asm
            wsock32.def
            )

add_library(Northstar::loader_wsock32_proxy ALIAS loader_wsock32_proxy)

target_link_libraries(loader_wsock32_proxy PRIVATE
                      ${CMAKE_CURRENT_SOURCE_DIR}/include/MinHook.x64.lib
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

target_include_directories(loader_wsock32_proxy PRIVATE
                           include
                           )

target_precompile_headers(loader_wsock32_proxy PRIVATE pch.h)

target_compile_definitions(loader_wsock32_proxy PRIVATE
                           UNICODE
                           _UNICODE
                           )

set_target_properties(loader_wsock32_proxy PROPERTIES
                      OUTPUT_NAME wsock32
                      )

if(MSVC)
	set_target_properties(loader_wsock32_proxy PROPERTIES
	                      LINK_FLAGS "/MANIFESTUAC:NO /SUBSYSTEM:WINDOWS"
	                      )
endif()
