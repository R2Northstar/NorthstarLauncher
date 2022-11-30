add_library(NorthstarDLL SHARED
            audio.cpp
            audio.h
            bansystem.cpp
            bansystem.h
            bitbuf.h
            bits.cpp
            bits.h
            buildainfile.cpp
            chatcommand.cpp
            clientauthhooks.cpp
            clientchathooks.cpp
            clientruihooks.cpp
            clientvideooverrides.cpp
            color.cpp
            color.h
            concommand.cpp
            concommand.h
            convar.cpp
            convar.h
            crashhandler.cpp
            crashhandler.h
            cvar.cpp
            cvar.h
            debugoverlay.cpp
            dedicated.cpp
            dedicated.h
            dedicatedmaterialsystem.cpp
            demofixes.cpp
            diskvmtfixes.cpp
            dllmain.cpp
            exploitfixes.cpp
            exploitfixes_lzss.cpp
            exploitfixes_utf8parser.cpp
            filesystem.cpp
            filesystem.h
            hooks.cpp
            hooks.h
            host.cpp
            hoststate.cpp
            hoststate.h
            kb_act.cpp
            keyvalues.cpp
            keyvalues.h
            languagehooks.cpp
            latencyflex.cpp
            limits.cpp
            limits.h
            localchatwriter.cpp
            localchatwriter.h
            logging.cpp
            logging.h
            loghooks.cpp
            loghooks.h
            main.h
            masterserver.cpp
            masterserver.h
            maxplayers.cpp
            maxplayers.h
            memalloc.cpp
            memalloc.h
            memory.cpp
            memory.h
            misccommands.cpp
            misccommands.h
            miscserverfixes.cpp
            miscserverscript.cpp
            modkeyvalues.cpp
            modlocalisation.cpp
            modmanager.cpp
            modmanager.h
            modpdef.cpp
            modscriptsrson.cpp
            ns_version.h
            nsprefix.cpp
            nsprefix.h
            pch.cpp
            pch.h
            playlist.cpp
            playlist.h
            plugin_abi.h
            plugins.cpp
            plugins.h
            printcommand.h
            printcommands.cpp
            printmaps.cpp
            printmaps.h
            r2client.cpp
            r2client.h
            r2engine.cpp
            r2engine.h
            r2server.cpp
            r2server.h
            rpakfilesystem.cpp
            rpakfilesystem.h
            runframe.cpp
            scriptbrowserhooks.cpp
            scriptdatatables.cpp
            scriptjson.cpp
            scriptmainmenupromos.cpp
            scriptmodmenu.cpp
            scriptserverbrowser.cpp
            scriptservertoclientstringcommand.cpp
            scriptuserinfo.cpp
            scriptutility.cpp
            serverauthentication.cpp
            serverauthentication.h
            serverchathooks.cpp
            serverchathooks.h
            serverpresence.cpp
            serverpresence.h
            sourceconsole.cpp
            sourceconsole.h
            sourceinterface.cpp
            sourceinterface.h
            squirrel.cpp
            squirrel.h
            squirrelautobind.cpp
            squirrelautobind.h
            squirrelclasstypes.h
            squirreldatatypes.h
            structs.h
            tier0.cpp
            tier0.h
            vector.h
            version.cpp
            version.h
            )

if(MSVC)
	target_sources(NorthstarDLL PRIVATE
	               audio_asm.asm
	               )
endif()

add_library(Northstar::DLL ALIAS NorthstarDLL)

target_link_libraries(NorthstarDLL PRIVATE
                      dbghelp.lib
                      Wldap32.lib
                      Normaliz.lib
                      version.lib
                      ws2_32.lib
                      )

if(MSVC)
	target_link_libraries(NorthstarDLL PRIVATE
	                      crypt32.lib
	                      ${CMAKE_CURRENT_SOURCE_DIR}/include/MinHook.x64.lib
	                      ${CMAKE_CURRENT_SOURCE_DIR}/include/libcurl/lib/libcurl_a.lib
	                      )
else()
	target_link_libraries(NorthstarDLL PRIVATE
	                      ${CMAKE_CURRENT_SOURCE_DIR}/include/minhook.a
	                      ${CMAKE_CURRENT_SOURCE_DIR}/include/libcurl/lib/libcurl.dll.a
	                      )
endif()

target_include_directories(NorthstarDLL PRIVATE
                           include
                           )

target_precompile_headers(NorthstarDLL PRIVATE pch.h)

target_compile_definitions(NorthstarDLL PRIVATE
                           UNICODE
                           _UNICODE
                           CURL_STATICLIB
                           )

set_target_properties(NorthstarDLL PROPERTIES
                      OUTPUT_NAME Northstar
                      )

if(MSVC)
	target_link_options(NorthstarDLL PRIVATE "/MANIFESTUAC:NO" "/SUBSYSTEM:WINDOWS")
endif()
