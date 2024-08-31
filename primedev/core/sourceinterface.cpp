#include "sourceinterface.h"
#include "logging/sourceconsole.h"

AUTOHOOK_INIT()

// really wanted to do a modular callback system here but honestly couldn't be bothered so hardcoding stuff for now: todo later

static void*(__fastcall* o_pClientCreateInterface)(const char* pName, const int* pReturnCode) = nullptr;
static void* __fastcall h_ClientCreateInterface(const char* pName, const int* pReturnCode)
{
	void* ret = o_pClientCreateInterface(pName, pReturnCode);
	spdlog::info("CreateInterface CLIENT {}", pName);

	if (!strcmp(pName, "GameClientExports001"))
		InitialiseConsoleOnInterfaceCreation();

	return ret;
}

// clang-format off
AUTOHOOK_PROCADDRESS(ServerCreateInterface, server.dll, CreateInterface, 
void*, __fastcall, (const char* pName, const int* pReturnCode))
// clang-format on
{
	void* ret = ServerCreateInterface(pName, pReturnCode);
	spdlog::info("CreateInterface SERVER {}", pName);

	return ret;
}

// clang-format off
AUTOHOOK_PROCADDRESS(EngineCreateInterface, engine.dll, CreateInterface, 
void*, __fastcall, (const char* pName, const int* pReturnCode))
// clang-format on
{
	void* ret = EngineCreateInterface(pName, pReturnCode);
	spdlog::info("CreateInterface ENGINE {}", pName);

	return ret;
}

// clang-format off
ON_DLL_LOAD("client.dll", ClientInterface, (CModule module))
{
	AUTOHOOK_DISPATCH_MODULE(client.dll)
	o_pClientCreateInterface = module.GetExportedFunction("CreateInterface").RCast<decltype(o_pClientCreateInterface)>();
	HookAttach(&(PVOID&)o_pClientCreateInterface, (PVOID)h_ClientCreateInterface);
}
ON_DLL_LOAD("server.dll", ServerInterface, (CModule module)) {AUTOHOOK_DISPATCH_MODULE(server.dll)}
ON_DLL_LOAD("engine.dll", EngineInterface, (CModule module)) {AUTOHOOK_DISPATCH_MODULE(engine.dll)}
// clang-format on
