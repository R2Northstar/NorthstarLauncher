#pragma once
#include <string>
#include <iostream>

void InstallInitialHooks();

typedef void (*DllLoadCallbackFuncType)(HMODULE moduleAddress);
void AddDllLoadCallback(std::string dll, DllLoadCallbackFuncType callback, std::string tag = "", std::string reliesOn = "");
void AddDllLoadCallbackForDedicatedServer(
	std::string dll, DllLoadCallbackFuncType callback, std::string tag = "", std::string reliesOn = "");
void AddDllLoadCallbackForClient(std::string dll, DllLoadCallbackFuncType callback, std::string tag = "", std::string reliesOn = "");

void CallAllPendingDLLLoadCallbacks();

// new dll load callback stuff
enum class eDllLoadCallbackSide
{
	UNSIDED,
	CLIENT,
	DEDICATED_SERVER
};

class __dllLoadCallback
{
  public:
	__dllLoadCallback() = delete;
	__dllLoadCallback(
		eDllLoadCallbackSide side,
		const std::string dllName,
		DllLoadCallbackFuncType callback,
		std::string uniqueStr,
		std::string reliesOn);
};

#define CONCAT_(x, y, z) x##y##z
#define CONCAT(x, y, z) CONCAT_(x, y, z)
#define __STR(s) #s

#define __ON_DLL_LOAD(dllName, func, side, counter, uniquestr, reliesOn) \
void CONCAT(__callbackFunc, uniquestr, counter) func \
__dllLoadCallback CONCAT(__dllLoadCallbackInstance, uniquestr, counter)(side, dllName, CONCAT(__callbackFunc, uniquestr, counter), __STR(uniquestr), reliesOn); 

#define ON_DLL_LOAD(dllName, uniquestr, func) __ON_DLL_LOAD(dllName, func, eDllLoadCallbackSide::UNSIDED, __LINE__, uniquestr, "")
#define ON_DLL_LOAD_RELIESON(dllName, uniquestr, reliesOn, func) __ON_DLL_LOAD(dllName, func, eDllLoadCallbackSide::UNSIDED, __LINE__, uniquestr, __STR(reliesOn))
#define ON_DLL_LOAD_CLIENT(dllName, uniquestr, func) __ON_DLL_LOAD(dllName, func, eDllLoadCallbackSide::CLIENT, __LINE__, uniquestr, "")
#define ON_DLL_LOAD_CLIENT_RELIESON(dllName, uniquestr, reliesOn,  func) __ON_DLL_LOAD(dllName, func, eDllLoadCallbackSide::CLIENT, __LINE__, uniquestr, __STR(reliesOn))
#define ON_DLL_LOAD_DEDI(dllName, uniquestr, func) __ON_DLL_LOAD(dllName, func, eDllLoadCallbackSide::DEDICATED_SERVER, __LINE__, uniquestr, "")
#define ON_DLL_LOAD_DEDI_RELIESON(dllName, uniquestr, reliesOn, func) __ON_DLL_LOAD(dllName, func, eDllLoadCallbackSide::DEDICATED_SERVER, __LINE__, uniquestr, __STR(reliesOn))
