#include "sourceinterface.h"
#include "logging/sourceconsole.h"

AUTOHOOK_INIT()

// really wanted to do a modular callback system here but honestly couldn't be bothered so hardcoding stuff for now: todo later

// clang-format off
AUTOHOOK_PROCADDRESS(ClientCreateInterface, client.dll, CreateInterface, 
void*, __fastcall, (const char* pName, const int* pReturnCode))
// clang-format on
{
	void* ret = ClientCreateInterface(pName, pReturnCode);
	DevMsg(eLog::CLIENT, "CreateInterface CLIENT %s\n", pName);

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
	DevMsg(eLog::SERVER, "CreateInterface SERVER %s\n", pName);

	return ret;
}

// clang-format off
AUTOHOOK_PROCADDRESS(EngineCreateInterface, engine.dll, CreateInterface, 
void*, __fastcall, (const char* pName, const int* pReturnCode))
// clang-format on
{
	void* ret = EngineCreateInterface(pName, pReturnCode);
	DevMsg(eLog::ENGINE, "CreateInterface ENGINE %s\n", pName);

	return ret;
}

// clang-format off
ON_DLL_LOAD("client.dll", ClientInterface, (CModule module)) {AUTOHOOK_DISPATCH_MODULE(client.dll)}
ON_DLL_LOAD("server.dll", ServerInterface, (CModule module)) {AUTOHOOK_DISPATCH_MODULE(server.dll)}
ON_DLL_LOAD("engine.dll", EngineInterface, (CModule module)) {AUTOHOOK_DISPATCH_MODULE(engine.dll)}
// clang-format on
