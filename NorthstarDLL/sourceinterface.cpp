#include "pch.h"
#include "sourceinterface.h"
#include "sourceconsole.h"

AUTOHOOK_INIT()

// really wanted to do a modular callback system here but honestly couldn't be bothered so hardcoding stuff for now: todo later

AUTOHOOK_PROCADDRESS(ClientCreateInterface, client.dll, CreateInterface, 
void*,, (const char* pName, const int* pReturnCode))
{
	void* ret = ClientCreateInterface(pName, pReturnCode);
	spdlog::info("CreateInterface CLIENT {}", pName);

	if (!strcmp(pName, "GameClientExports001"))
		InitialiseConsoleOnInterfaceCreation();

	return ret;
}

AUTOHOOK_PROCADDRESS(ServerCreateInterface, server.dll, CreateInterface, 
void*,, (const char* pName, const int* pReturnCode))
{
	void* ret = ServerCreateInterface(pName, pReturnCode);
		spdlog::info("CreateInterface SERVER {}", pName);

	return ret;
}

AUTOHOOK_PROCADDRESS(EngineCreateInterface, engine.dll, CreateInterface, 
void*,, (const char* pName, const int* pReturnCode))
{
	void* ret = EngineCreateInterface(pName, pReturnCode);
		spdlog::info("CreateInterface ENGINE {}", pName);

	return ret;
}

ON_DLL_LOAD("client.dll", ClientInterface, (HMODULE baseAddress))
{
	AUTOHOOK_DISPATCH_MODULE(client.dll)
}

ON_DLL_LOAD("server.dll", ServerInterface, (HMODULE baseAddress))
{
	AUTOHOOK_DISPATCH_MODULE(server.dll)
}

ON_DLL_LOAD("engine.dll", EngineInterface, (HMODULE baseAddress))
{ 
	AUTOHOOK_DISPATCH_MODULE(engine.dll) 
}