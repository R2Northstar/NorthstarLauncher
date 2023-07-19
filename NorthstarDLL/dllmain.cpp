#include "dllmain.h"
#include "logging/logging.h"
#include "logging/crashhandler.h"
#include "core/memalloc.h"
#include "config/profile.h"
#include "plugins/plugin_abi.h"
#include "plugins/plugins.h"
#include "util/version.h"
#include "squirrel/squirrel.h"
#include "shared/gamepresence.h"
#include "server/serverpresence.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/error/en.h"

#include <string.h>
#include <filesystem>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_DETACH:
		SpdLog_Shutdown();
		break;
	}

	return TRUE;
}

bool InitialiseNorthstar()
{
	static bool bInitialised = false;
	if (bInitialised)
		return false;

	bInitialised = true;

	InitialiseNorthstarPrefix();

	// Checks if we can write into install directory
	SpdLog_PreInit();

	Console_Init();

	SpdLog_Init();
	SpdLog_CreateLoggers();

	// initialise the console if needed (-northstar needs this)
	// InitialiseConsole();
	// initialise logging before most other things so that they can use spdlog and it have the proper formatting
	// InitialiseLogging();
	InitialiseVersion();
	// CreateLogFiles();

	InitialiseCrashHandler();

	// Write launcher version to log
	StartupLog();

	InstallInitialHooks();

	g_pServerPresence = new ServerPresenceManager();

	g_pGameStatePresence = new GameStatePresence();
	g_pPluginManager = new PluginManager();
	g_pPluginManager->LoadPlugins();

	InitialiseSquirrelManagers();

	// Fix some users' failure to connect to respawn datacenters
	SetEnvironmentVariableA("OPENSSL_ia32cap", "~0x200000200000000");

	curl_global_init_mem(CURL_GLOBAL_DEFAULT, _malloc_base, _free_base, _realloc_base, _strdup_base, _calloc_base);

	// run callbacks for any libraries that are already loaded by now
	CallAllPendingDLLLoadCallbacks();

	return true;
}
