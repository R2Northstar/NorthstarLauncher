# NorthstarDLL

find_package(minhook REQUIRED)
find_package(libcurl REQUIRED)
find_package(ZLIB REQUIRED)
find_package(LibUV REQUIRED)
find_package(minizip REQUIRED)
find_package(silver-bun REQUIRED)
find_package(uWebSockets REQUIRED)
find_package(OpenSSL REQUIRED)

add_library(
    NorthstarDLL SHARED
    "resources.rc"
    "client/audio.cpp"
    "client/audio.h"
    "client/chatcommand.cpp"
    "client/clientauthhooks.cpp"
    "client/clientruihooks.cpp"
    "client/clientvideooverrides.cpp"
    "client/debugoverlay.cpp"
    "client/demofixes.cpp"
    "client/diskvmtfixes.cpp"
    "client/entity_client_tools.cpp"
    "client/languagehooks.cpp"
    "client/latencyflex.cpp"
    "client/localchatwriter.cpp"
    "client/localchatwriter.h"
    "client/modlocalisation.cpp"
    "client/r2client.cpp"
    "client/r2client.h"
    "client/rejectconnectionfixes.cpp"
    "config/profile.cpp"
    "config/profile.h"
    "core/convar/concommand.cpp"
    "core/convar/concommand.h"
    "core/convar/convar.cpp"
    "core/convar/convar.h"
    "core/convar/cvar.cpp"
    "core/convar/cvar.h"
    "core/filesystem/filesystem.cpp"
    "core/filesystem/filesystem.h"
    "core/filesystem/rpakfilesystem.cpp"
    "core/filesystem/rpakfilesystem.h"
    "core/math/bitbuf.h"
    "core/math/bits.cpp"
    "core/math/bits.h"
    "core/math/color.cpp"
    "core/math/color.h"
    "core/math/math_pfns.h"
    "core/math/vector.h"
    "core/math/vplane.h"
    "core/hooks.cpp"
    "core/hooks.h"
    "core/macros.h"
    "core/memalloc.cpp"
    "core/memalloc.h"
    "core/sourceinterface.cpp"
    "core/tier0.cpp"
    "core/tier0.h"
    "core/tier1.cpp"
    "core/tier1.h"
    "dedicated/dedicated.cpp"
    "dedicated/dedicated.h"
    "dedicated/dedicatedlogtoclient.cpp"
    "dedicated/dedicatedlogtoclient.h"
    "dedicated/dedicatedmaterialsystem.cpp"
    "engine/gl_matsysiface.cpp"
    "engine/host.cpp"
    "engine/hoststate.cpp"
    "engine/hoststate.h"
    "engine/r2engine.cpp"
    "engine/r2engine.h"
    "engine/runframe.cpp"
    "game/client/clientmode_shared.cpp"
    "imgui/imgui_ws_test.cpp"
    "imgui/imgui_ws_test.h"
    "imgui/imgui_menu.cpp"
    "imgui/imgui_menu.h"
    "imgui/imgui/imgui.cpp"
    "imgui/imgui/imgui.h"
    "imgui/imgui/imgui_demo.cpp"
    "imgui/imgui/imgui_draw.cpp"
    "imgui/imgui/imgui_internal.h"
    "imgui/imgui/imgui_tables.cpp"
    "imgui/imgui/imgui_widgets.cpp"
    "imgui/imgui/backends/imgui_impl_dx11.cpp"
    "imgui/imgui/backends/imgui_impl_dx11.h"
    "imgui/imgui/backends/imgui_impl_win32.cpp"
    "imgui/imgui/backends/imgui_impl_win32.h"
    "imgui/imgui-ws/imgui-ws.cpp"
    "imgui/imgui-ws/imgui-ws.h"
    "imgui/imgui-ws/compressor-xor-rle-per-draw-list-with-vtx-offset.cpp"
    "imgui/imgui-ws/compressor-xor-rle-per-draw-list.cpp"
    "imgui/imgui-ws/imgui-draw-data-compressor.h"
    "imgui/imgui-ws/incppect/incppect.cpp"
    "imgui/imgui-ws/incppect/incppect.h"
    "logging/crashhandler.cpp"
    "logging/crashhandler.h"
    "logging/logging.cpp"
    "logging/logging.h"
    "logging/loghooks.cpp"
    "logging/loghooks.h"
    "logging/sourceconsole.cpp"
    "logging/sourceconsole.h"
    "masterserver/masterserver.cpp"
    "masterserver/masterserver.h"
    "mods/autodownload/moddownloader.h"
    "mods/autodownload/moddownloader.cpp"
    "mods/compiled/kb_act.cpp"
    "mods/compiled/modkeyvalues.cpp"
    "mods/compiled/modpdef.cpp"
    "mods/compiled/modscriptsrson.cpp"
    "mods/mod.cpp"
    "mods/mod.h"
    "mods/modmanager.cpp"
    "mods/modmanager.h"
    "mods/modsavefiles.cpp"
    "mods/modsavefiles.h"
    "plugins/interfaces/interface.h"
    "plugins/interfaces/interface.cpp"
    "plugins/interfaces/sys/ISys.h"
    "plugins/interfaces/sys/ISys.cpp"
    "plugins/interfaces/IPluginId.h"
    "plugins/interfaces/IPluginCallbacks.h"
    "plugins/plugins.cpp"
    "plugins/plugins.h"
    "plugins/pluginmanager.h"
    "plugins/pluginmanager.cpp"
    "scripts/client/clientchathooks.cpp"
    "scripts/client/cursorposition.cpp"
    "scripts/client/scriptbrowserhooks.cpp"
    "scripts/client/scriptimgui.cpp"
    "scripts/client/scriptmainmenupromos.cpp"
    "scripts/client/scriptmodmenu.cpp"
    "scripts/client/scriptoriginauth.cpp"
    "scripts/client/scriptserverbrowser.cpp"
    "scripts/client/scriptservertoclientstringcommand.cpp"
    "scripts/server/miscserverfixes.cpp"
    "scripts/server/miscserverscript.cpp"
    "scripts/server/scriptuserinfo.cpp"
    "scripts/scriptdatatables.cpp"
    "scripts/scripthttprequesthandler.cpp"
    "scripts/scripthttprequesthandler.h"
    "scripts/scriptjson.cpp"
    "scripts/scriptjson.h"
    "scripts/scriptutility.cpp"
    "server/auth/bansystem.cpp"
    "server/auth/bansystem.h"
    "server/auth/serverauthentication.cpp"
    "server/auth/serverauthentication.h"
    "server/alltalk.cpp"
    "server/ai_helper.cpp"
    "server/ai_helper.h"
    "server/ai_navmesh.cpp"
    "server/ai_navmesh.h"
    "server/buildainfile.cpp"
    "server/r2server.cpp"
    "server/r2server.h"
    "server/serverchathooks.cpp"
    "server/serverchathooks.h"
    "server/servernethooks.cpp"
    "server/serverpresence.cpp"
    "server/serverpresence.h"
    "shared/exploit_fixes/exploitfixes.cpp"
    "shared/exploit_fixes/exploitfixes_lzss.cpp"
    "shared/exploit_fixes/exploitfixes_utf8parser.cpp"
    "shared/exploit_fixes/ns_limits.cpp"
    "shared/exploit_fixes/ns_limits.h"
    "shared/keyvalues.cpp"
    "shared/keyvalues.h"
    "shared/maxplayers.cpp"
    "shared/maxplayers.h"
    "shared/misccommands.cpp"
    "shared/misccommands.h"
    "shared/playlist.cpp"
    "shared/playlist.h"
    "squirrel/squirrel.cpp"
    "squirrel/squirrel.h"
    "squirrel/squirrelautobind.cpp"
    "squirrel/squirrelautobind.h"
    "squirrel/squirrelclasstypes.h"
    "util/printcommands.cpp"
    "util/printcommands.h"
    "util/printmaps.cpp"
    "util/printmaps.h"
    "util/utils.cpp"
    "util/utils.h"
    "util/version.cpp"
    "util/version.h"
    "util/wininfo.cpp"
    "util/wininfo.h"
    "vscript/languages/squirrel_re/include/squirrel.h"
    "vscript/languages/squirrel_re/squirrel/sqarray.h"
    "vscript/languages/squirrel_re/squirrel/sqclosure.h"
    "vscript/languages/squirrel_re/squirrel/sqcompiler.h"
    "vscript/languages/squirrel_re/squirrel/sqfunctionproto.h"
    "vscript/languages/squirrel_re/squirrel/sqlexer.h"
    "vscript/languages/squirrel_re/squirrel/sqobject.h"
    "vscript/languages/squirrel_re/squirrel/sqopcodes.h"
    "vscript/languages/squirrel_re/squirrel/sqstate.h"
    "vscript/languages/squirrel_re/squirrel/sqstring.h"
    "vscript/languages/squirrel_re/squirrel/sqstruct.h"
    "vscript/languages/squirrel_re/squirrel/sqtable.h"
    "vscript/languages/squirrel_re/squirrel/squserdata.h"
    "vscript/languages/squirrel_re/squirrel/sqvector.h"
    "vscript/languages/squirrel_re/squirrel/sqvm.h"
    "vscript/languages/squirrel_re/vsquirrel.h"
    "vscript/vscript.h"
    "windows/libsys.cpp"
    "windows/libsys.h"
    "dllmain.cpp"
    "ns_version.h"
    "Northstar.def"
    )

target_include_directories(
    NorthstarDLL
    PRIVATE "imgui/imgui"
    )

# for the autogenerated files
set(WEB_EMBED_OUTPUT_FILES "imgui/imgui-ws/common.h" "imgui/imgui-ws/incppect/common.h")
set(WEB_EMBED_INPUT_FILES "imgui/imgui-ws/imgui-ws.js" "imgui/imgui-ws/index.html" "imgui/imgui-ws/incppect/incppect.js")
add_custom_command(
    OUTPUT ${WEB_EMBED_OUTPUT_FILES}
    COMMAND ${CMAKE_COMMAND} -D SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR} -P ${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui-ws/embed_web.cmake
    DEPENDS ${WEB_EMBED_INPUT_FILES}
    COMMENT "Embedding web files into headers."
    )

add_custom_target(embed_web ALL DEPENDS ${WEB_EMBED_OUTPUT_FILES})
add_dependencies(NorthstarDLL embed_web)

# global compile defs
add_compile_definitions(
    LIBUS_USE_OPENSSL
    LIBUS_USE_LIBUV
    )

target_link_libraries(
    NorthstarDLL
    PRIVATE minhook
            libcurl
            minizip
            silver-bun
            ws2_32.lib
            crypt32.lib
            cryptui.lib
            dbghelp.lib
            wldap32.lib
            normaliz.lib
            bcrypt.lib
            version.lib
            uWebSockets
    )

target_precompile_headers(
    NorthstarDLL
    PRIVATE
    pch.h
    )

target_compile_definitions(
    NorthstarDLL
    PRIVATE UNICODE
            _UNICODE
            CURL_STATICLIB
            IMGUI_DEFINE_MATH_OPERATORS
    )

set_target_properties(
    NorthstarDLL
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${NS_BINARY_DIR}
               OUTPUT_NAME Northstar
               LINK_FLAGS "/MANIFEST:NO /DEBUG"
    )
