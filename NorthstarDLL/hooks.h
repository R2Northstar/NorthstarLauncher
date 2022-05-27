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

#define __CONCAT3(x, y, z) x##y##z
#define CONCAT3(x, y, z) __CONCAT3(x, y, z)
#define __CONCAT2(x, y) x##y
#define CONCAT2(x, y) __CONCAT2(x, y)
#define __STR(s) #s

// adds a callback to be called when a given dll is loaded, for creating hooks and such
#define __ON_DLL_LOAD(dllName, func, side, uniquestr, reliesOn) \
namespace { __dllLoadCallback CONCAT2(__dllLoadCallbackInstance, __LINE__)(side, dllName, func, __STR(uniquestr), reliesOn); }

#define ON_DLL_LOAD(dllName, uniquestr, func) __ON_DLL_LOAD(dllName, func, eDllLoadCallbackSide::UNSIDED, uniquestr, "")
#define ON_DLL_LOAD_RELIESON(dllName, uniquestr, reliesOn, func) __ON_DLL_LOAD(dllName, func, eDllLoadCallbackSide::UNSIDED, uniquestr, __STR(reliesOn))
#define ON_DLL_LOAD_CLIENT(dllName, uniquestr, func) __ON_DLL_LOAD(dllName, func, eDllLoadCallbackSide::CLIENT, uniquestr, "")
#define ON_DLL_LOAD_CLIENT_RELIESON(dllName, uniquestr, reliesOn,  func) __ON_DLL_LOAD(dllName, func, eDllLoadCallbackSide::CLIENT, uniquestr, __STR(reliesOn))
#define ON_DLL_LOAD_DEDI(dllName, uniquestr, func) __ON_DLL_LOAD(dllName, func, eDllLoadCallbackSide::DEDICATED_SERVER, uniquestr, "")
#define ON_DLL_LOAD_DEDI_RELIESON(dllName, uniquestr, reliesOn, func) __ON_DLL_LOAD(dllName, func, eDllLoadCallbackSide::DEDICATED_SERVER, uniquestr, __STR(reliesOn))

// new macro hook stuff
class __autohook;

class __fileAutohook
{
  public:
	std::vector<__autohook*> hooks;

	void Dispatch();
	void DispatchForModule(const char* pModuleName);
};

// initialise autohooks for this file
#define AUTOHOOK_INIT() \
namespace { __fileAutohook __FILEAUTOHOOK; } \

// dispatch all autohooks in this file
#define AUTOHOOK_DISPATCH() \
__FILEAUTOHOOK.Dispatch(); \

#define AUTOHOOK_DISPATCH_MODULE(moduleName) \
__FILEAUTOHOOK.DispatchForModule(__STR(moduleName)); \

class __autohook
{
  public:
	enum AddressResolutionMode
	{
		OFFSET_STRING, // we're using a string that of the format dllname.dll + offset
		ABSOLUTE_ADDR, // we're using an absolute address, we don't need to process it at all
		PROCADDRESS // resolve using GetModuleHandle and GetProcAddress
	};

	char* pFuncName;

	LPVOID pHookFunc;
	LPVOID* ppOrigFunc;

	// address resolution props
	AddressResolutionMode iAddressResolutionMode;
	char* pAddrString = nullptr; // for OFFSET_STRING
	LPVOID iAbsoluteAddress = nullptr; // for ABSOLUTE_ADDR
	char* pModuleName; // for PROCADDRESS
	char* pProcName; // for PROCADDRESS

  public: 
	__autohook() = delete;

	__autohook(__fileAutohook* autohook, const char* funcName, LPVOID absoluteAddress, LPVOID* orig, LPVOID func)
		: pHookFunc(func), ppOrigFunc(orig), iAbsoluteAddress(absoluteAddress)
	{
		iAddressResolutionMode = ABSOLUTE_ADDR;

		const int iFuncNameStrlen = strlen(funcName) + 1;
		pFuncName = new char[iFuncNameStrlen];
		memcpy(pFuncName, funcName, iFuncNameStrlen);

		autohook->hooks.push_back(this);
	}

	__autohook(__fileAutohook* autohook, const char* funcName, const char* addrString, LPVOID* orig, LPVOID func)
		: pHookFunc(func), ppOrigFunc(orig)
	{
		iAddressResolutionMode = OFFSET_STRING;

		const int iFuncNameStrlen = strlen(funcName) + 1;
		pFuncName = new char[iFuncNameStrlen];
		memcpy(pFuncName, funcName, iFuncNameStrlen);

		const int iAddrStrlen = strlen(addrString) + 1;
		pAddrString = new char[iAddrStrlen];
		memcpy(pAddrString, addrString, iAddrStrlen);

		autohook->hooks.push_back(this);
	}

	__autohook(__fileAutohook* autohook, const char* funcName, const char* moduleName, const char* procName, LPVOID* orig, LPVOID func)
		: pHookFunc(func), ppOrigFunc(orig)
	{
		iAddressResolutionMode = PROCADDRESS;

		const int iFuncNameStrlen = strlen(funcName) + 1;
		pFuncName = new char[iFuncNameStrlen];
		memcpy(pFuncName, funcName, iFuncNameStrlen);

		const int iModuleNameStrlen = strlen(moduleName) + 1;
		pModuleName = new char[iModuleNameStrlen];
		memcpy(pModuleName, moduleName, iModuleNameStrlen);

		const int iProcNameStrlen = strlen(procName) + 1;
		pProcName = new char[iProcNameStrlen];
		memcpy(pProcName, procName, iProcNameStrlen);

		autohook->hooks.push_back(this);
	}

	~__autohook()
	{
		delete[] pFuncName;

		if (pAddrString)
			delete[] pAddrString;

		if (pModuleName)
			delete[] pModuleName;

		if (pProcName)
			delete[] pProcName;
	}

	void Dispatch() 
	{
		LPVOID targetAddr = nullptr;

		// determine the address of the function we're hooking
		switch (iAddressResolutionMode)
		{
		case ABSOLUTE_ADDR:
		{
			targetAddr = iAbsoluteAddress;
			break;
		}

		case OFFSET_STRING:
		{
			// in the format server.dll + 0xDEADBEEF
			int iDllNameEnd = 0;
			for (; !isspace(pAddrString[iDllNameEnd]) && pAddrString[iDllNameEnd] != '+'; iDllNameEnd++);

			char* pModuleName = new char[iDllNameEnd + 1];
			memcpy(pModuleName, pAddrString, iDllNameEnd);
			pModuleName[iDllNameEnd] = '\0';

			// get the module address
			const HMODULE pModuleAddr = GetModuleHandleA(pModuleName);

			if (!pModuleAddr)
				break;

			// get the offset string
			uintptr_t iOffset = 0;

			int iOffsetBegin = iDllNameEnd;
			int iOffsetEnd = strlen(pAddrString);

			// seek until we hit the start of the number offset
			for (; !(pAddrString[iOffsetBegin] >= '0' && pAddrString[iOffsetBegin] <= '9') && pAddrString[iOffsetBegin]; iOffsetBegin++);

			bool bIsHex = pAddrString[iOffsetBegin] == '0' && (pAddrString[iOffsetBegin + 1] == 'X' || pAddrString[iOffsetBegin + 1] == 'x');
			if (bIsHex)
				iOffset = std::stoi(pAddrString + iOffsetBegin + 2, 0, 16);
			else
				iOffset = std::stoi(pAddrString + iOffsetBegin);

			targetAddr = (LPVOID)((uintptr_t)pModuleAddr + iOffset);
			break;
		}

		case PROCADDRESS:
		{
			targetAddr = GetProcAddress(GetModuleHandleA(pModuleName), pProcName);
			break;
		}
		}

		if (MH_CreateHook(targetAddr, pHookFunc, ppOrigFunc) == MH_OK)
		{
			if (MH_EnableHook(targetAddr) == MH_OK)
				spdlog::info("Enabling hook {}", pFuncName);
			else
				spdlog::error("MH_EnableHook failed for function {}", pFuncName);
		}
		else
			spdlog::error("MH_CreateHook failed for function {}", pFuncName);
	}
};

// hook a function at a given offset from a dll to be dispatched with AUTOHOOK_DISPATCH()
#define AUTOHOOK(name, addrString, type, callingConvention, args, func) \
namespace { \
type(*callingConvention name) args; \
type callingConvention CONCAT2(__autohookfunc, name) args func \
__autohook CONCAT2(__autohook, __LINE__)(&__FILEAUTOHOOK, __STR(name), __STR(addrString), (LPVOID*)&name, (LPVOID)CONCAT2(__autohookfunc, name)); \
} \

// hook a function at a given absolute constant address to be dispatched with AUTOHOOK_DISPATCH()
#define AUTOHOOK_ABSOLUTEADDR(name, addr, type, callingConvention, args, func) \
namespace { \
type(*callingConvention name) args; \
type callingConvention CONCAT2(__autohookfunc, name) args func \
__autohook CONCAT2(__autohook, __LINE__)(&__FILEAUTOHOOK, __STR(name), addr, (LPVOID*)&name, (LPVOID)CONCAT2(__autohookfunc, name)); \
} \

// hook a function at a given module and exported function to be dispatched with AUTOHOOK_DISPATCH()
#define AUTOHOOK_PROCADDRESS(name, moduleName, procName, type, callingConvention, args, func) \
namespace { \
type(*callingConvention name) args; \
type callingConvention CONCAT2(__autohookfunc, name) args func \
__autohook CONCAT2(__autohook, __LINE__)(&__FILEAUTOHOOK, __STR(name), __STR(moduleName), __STR(procName), (LPVOID*)&name, (LPVOID)CONCAT2(__autohookfunc, name)); \
} \

class ManualHook
{
  public:
	char* pFuncName;

	LPVOID pHookFunc;
	LPVOID* ppOrigFunc;

  public:
	ManualHook() = delete;
	ManualHook(const char* funcName, LPVOID func);
	ManualHook(const char* funcName, LPVOID* orig, LPVOID func);
	bool Dispatch(LPVOID addr, LPVOID* orig = nullptr);
};

// hook a function to be dispatched manually later
#define HOOK(varName, originalFunc, type, callingConvention, args, func) \
namespace { \
type(*callingConvention originalFunc) args; \
type callingConvention CONCAT2(__manualhookfunc, varName) args func \
} \
ManualHook varName = ManualHook(__STR(varName), (LPVOID*)&originalFunc, (LPVOID)CONCAT2(__manualhookfunc, varName)); \

#define HOOK_NOORIG(varName, type, callingConvention, args, func) \
namespace { \
type callingConvention CONCAT2(__manualhookfunc, varName) args func \
} \
ManualHook varName = ManualHook(__STR(varName), (LPVOID)CONCAT2(__manualhookfunc, varName)); \