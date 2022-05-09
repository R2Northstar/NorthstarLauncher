#include "pch.h"
#include "sourceinterface.h"
#include "hooks.h"
#include "hookutils.h"

#include "sourceconsole.h"
#include "context.h"
#include "convar.h"
#include <iostream>

// really wanted to do a modular callback system here but honestly couldn't be bothered so hardcoding stuff for now: todo later

CreateInterfaceFn clientCreateInterfaceOriginal;
void* ClientCreateInterfaceHook(const char* pName, int* pReturnCode)
{
	void* ret = clientCreateInterfaceOriginal(pName, pReturnCode);
	spdlog::info("CreateInterface CLIENT {}", pName);

	if (!strcmp(pName, "GameClientExports001"))
		InitialiseConsoleOnInterfaceCreation();

	return ret;
}

CreateInterfaceFn serverCreateInterfaceOriginal;
void* ServerCreateInterfaceHook(const char* pName, int* pReturnCode)
{
	void* ret = serverCreateInterfaceOriginal(pName, pReturnCode);
	spdlog::info("CreateInterface SERVER {}", pName);

	return ret;
}

CreateInterfaceFn engineCreateInterfaceOriginal;
void* EngineCreateInterfaceHook(const char* pName, int* pReturnCode)
{
	void* ret = engineCreateInterfaceOriginal(pName, pReturnCode);
	spdlog::info("CreateInterface ENGINE {}", pName);

	return ret;
}

ON_DLL_LOAD("client.dll", ClientInterface, (HMODULE baseAddress)
{
	HookEnabler hook;
	ENABLER_CREATEHOOK(
		hook,
		GetProcAddress(baseAddress, "CreateInterface"),
		&ClientCreateInterfaceHook,
		reinterpret_cast<LPVOID*>(&clientCreateInterfaceOriginal));
})

ON_DLL_LOAD("server.dll", ServerInterface, (HMODULE baseAddress)
{
	HookEnabler hook;
	ENABLER_CREATEHOOK(
		hook,
		GetProcAddress(baseAddress, "CreateInterface"),
		&ServerCreateInterfaceHook,
		reinterpret_cast<LPVOID*>(&serverCreateInterfaceOriginal));
})

ON_DLL_LOAD("engine.dll", EngineInterface, (HMODULE baseAddress)
{
	HookEnabler hook;
	ENABLER_CREATEHOOK(
		hook,
		GetProcAddress(baseAddress, "CreateInterface"),
		&EngineCreateInterfaceHook,
		reinterpret_cast<LPVOID*>(&engineCreateInterfaceOriginal));
})