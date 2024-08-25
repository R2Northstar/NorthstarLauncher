#include "sourceinterface.h"
#include "logging/sourceconsole.h"

AUTOHOOK_INIT()

// really wanted to do a modular callback system here but honestly couldn't be bothered so hardcoding stuff for now: todo later

AUTOHOOK_PROCADDRESS(ClientCreateInterface, client.dll, CreateInterface, 
void*, __fastcall, (const char* pName, const int* pReturnCode))
{
	void* ret = ClientCreateInterface(pName, pReturnCode);
	spdlog::info("CreateInterface CLIENT {}", pName);

	if (!strcmp(pName, "GameClientExports001"))
		InitialiseConsoleOnInterfaceCreation();

	return ret;
}

AUTOHOOK_PROCADDRESS(ServerCreateInterface, server.dll, CreateInterface, 
void*, __fastcall, (const char* pName, const int* pReturnCode))
{
	void* ret = ServerCreateInterface(pName, pReturnCode);
	spdlog::info("CreateInterface SERVER {}", pName);

	return ret;
}

AUTOHOOK_PROCADDRESS(EngineCreateInterface, engine.dll, CreateInterface, 
void*, __fastcall, (const char* pName, const int* pReturnCode))
{
	void* ret = EngineCreateInterface(pName, pReturnCode);
	spdlog::info("CreateInterface ENGINE {}", pName);

	return ret;
}

ON_DLL_LOAD("client.dll", ClientInterface, (CModule module)) {AUTOHOOK_DISPATCH_MODULE(client.dll)}
ON_DLL_LOAD("server.dll", ServerInterface, (CModule module)) {AUTOHOOK_DISPATCH_MODULE(server.dll)}
ON_DLL_LOAD("engine.dll", EngineInterface, (CModule module)) {AUTOHOOK_DISPATCH_MODULE(engine.dll)}
