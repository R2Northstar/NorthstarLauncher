#include "pch.h"
#include "sourceinterface.h"
#include "sourceconsole.h"

// really wanted to do a modular callback system here but honestly couldn't be bothered so hardcoding stuff for now: todo later

CreateInterfaceFn ClientCreateInterfaceOriginal;
void* ClientCreateInterfaceHook(const char* pName, int* pReturnCode)
{
	void* ret = ClientCreateInterfaceOriginal(pName, pReturnCode);
	spdlog::info("CreateInterface CLIENT {}", pName);

	if (!strcmp(pName, "GameClientExports001"))
		InitialiseConsoleOnInterfaceCreation();

	return ret;
}

CreateInterfaceFn ServerCreateInterfaceOriginal;
void* ServerCreateInterfaceHook(const char* pName, int* pReturnCode)
{
	void* ret = ServerCreateInterfaceOriginal(pName, pReturnCode);
	spdlog::info("CreateInterface SERVER {}", pName);

	return ret;
}

CreateInterfaceFn EngineCreateInterfaceOriginal;
void* EngineCreateInterfaceHook(const char* pName, int* pReturnCode)
{
	void* ret = EngineCreateInterfaceOriginal(pName, pReturnCode);
	spdlog::info("CreateInterface ENGINE {}", pName);

	return ret;
}

ON_DLL_LOAD("client.dll", ClientInterface, [](HMODULE baseAddress)
{
	HookEnabler hook;
	ENABLER_CREATEHOOK(
		hook,
		GetProcAddress(baseAddress, "CreateInterface"),
		&ClientCreateInterfaceHook,
		reinterpret_cast<LPVOID*>(&ClientCreateInterfaceOriginal));
})

ON_DLL_LOAD("server.dll", ServerInterface, [](HMODULE baseAddress)
{
	HookEnabler hook;
	ENABLER_CREATEHOOK(
		hook,
		GetProcAddress(baseAddress, "CreateInterface"),
		&ServerCreateInterfaceHook,
		reinterpret_cast<LPVOID*>(&ServerCreateInterfaceOriginal));
})

ON_DLL_LOAD("engine.dll", EngineInterface, [](HMODULE baseAddress)
{
	HookEnabler hook;
	ENABLER_CREATEHOOK(
		hook,
		GetProcAddress(baseAddress, "CreateInterface"),
		&EngineCreateInterfaceHook,
		reinterpret_cast<LPVOID*>(&EngineCreateInterfaceOriginal));
})