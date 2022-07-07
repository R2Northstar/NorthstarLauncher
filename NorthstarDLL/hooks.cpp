#include "pch.h"
#include "dedicated.h"

#include <iostream>
#include <wchar.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <Psapi.h>

namespace fs = std::filesystem;

AUTOHOOK_INIT()

// called from the ON_DLL_LOAD macros
__dllLoadCallback::__dllLoadCallback(
	eDllLoadCallbackSide side, const std::string dllName, DllLoadCallbackFuncType callback, std::string uniqueStr, std::string reliesOn)
{
	switch (side)
	{
		case eDllLoadCallbackSide::UNSIDED:
		{
			AddDllLoadCallback(dllName, callback, uniqueStr, reliesOn);
			break;
		}

		case eDllLoadCallbackSide::CLIENT:
		{
			AddDllLoadCallbackForClient(dllName, callback, uniqueStr, reliesOn);
			break;
		}

		case eDllLoadCallbackSide::DEDICATED_SERVER:
		{
			AddDllLoadCallbackForDedicatedServer(dllName, callback, uniqueStr, reliesOn);
			break;
		}
	}
}

void __fileAutohook::Dispatch()
{
	for (__autohook* hook : hooks)
		hook->Dispatch();
}

void __fileAutohook::DispatchForModule(const char* pModuleName)
{
	const int iModuleNameLen = strlen(pModuleName);

	for (__autohook* hook : hooks)
		if ((hook->iAddressResolutionMode == __autohook::OFFSET_STRING && !strncmp(pModuleName, hook->pAddrString, iModuleNameLen)) ||
			(hook->iAddressResolutionMode == __autohook::PROCADDRESS && !strcmp(pModuleName, hook->pModuleName)))
			hook->Dispatch();
}

ManualHook::ManualHook(const char* funcName, LPVOID func)
	: pHookFunc(func), ppOrigFunc(nullptr)
{
	const int iFuncNameStrlen = strlen(funcName);
	pFuncName = new char[iFuncNameStrlen];
	memcpy(pFuncName, funcName, iFuncNameStrlen);
}

ManualHook::ManualHook(const char* funcName, LPVOID* orig, LPVOID func) 
	: pHookFunc(func), ppOrigFunc(orig)
{
	const int iFuncNameStrlen = strlen(funcName);
	pFuncName = new char[iFuncNameStrlen];
	memcpy(pFuncName, funcName, iFuncNameStrlen);
}

bool ManualHook::Dispatch(LPVOID addr, LPVOID* orig)
{
	if (orig)
		ppOrigFunc = orig;

	if (MH_CreateHook(addr, pHookFunc, ppOrigFunc) == MH_OK)
	{
		if (MH_EnableHook(addr) == MH_OK)
		{
			spdlog::info("Enabling hook {}", pFuncName);
			return true;
		}
		else
			spdlog::error("MH_EnableHook failed for function {}", pFuncName);
	}
	else
		spdlog::error("MH_CreateHook failed for function {}", pFuncName);

	return false;
}

// dll load callback stuff
// this allows for code to register callbacks to be run as soon as a dll is loaded, mainly to allow for patches to be made on dll load
struct DllLoadCallback
{
	std::string dll;
	DllLoadCallbackFuncType callback;
	std::string tag;
	std::string reliesOn;
	bool called;
};

// HACK: declaring and initialising this vector at file scope crashes on debug builds due to static initialisation order
// using a static var like this ensures that the vector is initialised lazily when it's used
std::vector<DllLoadCallback>& GetDllLoadCallbacks()
{
	static std::vector<DllLoadCallback> vec = std::vector<DllLoadCallback>();
	return vec;
}

void AddDllLoadCallback(std::string dll, DllLoadCallbackFuncType callback, std::string tag, std::string reliesOn)
{
	DllLoadCallback& callbackStruct = GetDllLoadCallbacks().emplace_back();

	callbackStruct.dll = dll;
	callbackStruct.callback = callback;
	callbackStruct.tag = tag;
	callbackStruct.reliesOn = reliesOn;
	callbackStruct.called = false;
}

void AddDllLoadCallbackForDedicatedServer(
	std::string dll, DllLoadCallbackFuncType callback, std::string tag, std::string reliesOn)
{
	if (!IsDedicatedServer())
		return;

	AddDllLoadCallback(dll, callback, tag, reliesOn);
}

void AddDllLoadCallbackForClient(std::string dll, DllLoadCallbackFuncType callback, std::string tag, std::string reliesOn)
{
	if (IsDedicatedServer())
		return;

	AddDllLoadCallback(dll, callback, tag, reliesOn);
}

AUTOHOOK_ABSOLUTEADDR(_GetCommandLineA, GetCommandLineA,
LPSTR, WINAPI, ())
{
	static char* cmdlineModified;
	static char* cmdlineOrg;

	if (cmdlineOrg == nullptr || cmdlineModified == nullptr)
	{
		cmdlineOrg = _GetCommandLineA();
		bool isDedi = strstr(cmdlineOrg, "-dedicated"); // well, this one has to be a real argument
		bool ignoreStartupArgs = strstr(cmdlineOrg, "-nostartupargs");

		std::string args;
		std::ifstream cmdlineArgFile;

		// it looks like CommandLine() prioritizes parameters apprearing first, so we want the real commandline to take priority
		// not to mention that cmdlineOrg starts with the EXE path
		args.append(cmdlineOrg);
		args.append(" ");

		// append those from the file

		if (!ignoreStartupArgs)
		{

			cmdlineArgFile = std::ifstream(!isDedi ? "ns_startup_args.txt" : "ns_startup_args_dedi.txt");

			if (cmdlineArgFile)
			{
				std::stringstream argBuffer;
				argBuffer << cmdlineArgFile.rdbuf();
				cmdlineArgFile.close();

				// if some other command line option includes "-northstar" in the future then you have to refactor this check to check with
				// both either space after or ending with
				if (!isDedi && argBuffer.str().find("-northstar") != std::string::npos)
					MessageBoxA(
						NULL,
						"The \"-northstar\" command line option is NOT supposed to go into ns_startup_args.txt file!\n\nThis option is "
						"supposed to go into Origin/Steam game launch options, and then you are supposed to launch the original "
						"Titanfall2.exe "
						"rather than NorthstarLauncher.exe to make use of it.",
						"Northstar Warning",
						MB_ICONWARNING);

				args.append(argBuffer.str());
			}
		}

		auto len = args.length();
		cmdlineModified = new char[len + 1];
		if (!cmdlineModified)
		{
			spdlog::error("malloc failed for command line");
			return cmdlineOrg;
		}
		memcpy(cmdlineModified, args.c_str(), len + 1);

		spdlog::info("Command line: {}", cmdlineModified);
	}

	return cmdlineModified;
}

std::vector<std::string> calledTags;
void CallLoadLibraryACallbacks(LPCSTR lpLibFileName, HMODULE moduleAddress)
{
	while (true)
	{
		bool bDoneCalling = true;

		for (auto& callbackStruct : GetDllLoadCallbacks())
		{
			if (!callbackStruct.called && fs::path(lpLibFileName).filename() == fs::path(callbackStruct.dll).filename())
			{
				if (callbackStruct.reliesOn != "" &&
					std::find(calledTags.begin(), calledTags.end(), callbackStruct.reliesOn) == calledTags.end())
				{
					bDoneCalling = false;
					continue;
				}

				callbackStruct.callback(moduleAddress);
				calledTags.push_back(callbackStruct.tag);
				callbackStruct.called = true;
			}
		}

		if (bDoneCalling)
			break;
	}
}

void CallLoadLibraryWCallbacks(LPCWSTR lpLibFileName, HMODULE moduleAddress)
{
	while (true)
	{
		bool bDoneCalling = true;

		for (auto& callbackStruct : GetDllLoadCallbacks())
		{
			if (!callbackStruct.called && fs::path(lpLibFileName).filename() == fs::path(callbackStruct.dll).filename())
			{
				if (callbackStruct.reliesOn != "" &&
					std::find(calledTags.begin(), calledTags.end(), callbackStruct.reliesOn) == calledTags.end())
				{
					bDoneCalling = false;
					continue;
				}

				callbackStruct.callback(moduleAddress);
				calledTags.push_back(callbackStruct.tag);
				callbackStruct.called = true;
			}
		}

		if (bDoneCalling)
			break;
	}
}

void CallAllPendingDLLLoadCallbacks()
{
	HMODULE hMods[1024];
	HANDLE hProcess = GetCurrentProcess();
	DWORD cbNeeded;
	unsigned int i;

	// Get a list of all the modules in this process.
	if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
	{
		for (i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
		{
			wchar_t szModName[MAX_PATH];

			// Get the full path to the module's file.
			if (GetModuleFileNameExW(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR)))
			{
				CallLoadLibraryWCallbacks(szModName, hMods[i]);
			}
		}
	}
}

AUTOHOOK_ABSOLUTEADDR(_LoadLibraryExA, LoadLibraryExA, 
HMODULE, WINAPI, (LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags))
	{
	HMODULE moduleAddress = _LoadLibraryExA(lpLibFileName, hFile, dwFlags);

	if (moduleAddress)
		CallLoadLibraryACallbacks(lpLibFileName, moduleAddress);

	return moduleAddress;
}


AUTOHOOK_ABSOLUTEADDR(_LoadLibraryA, LoadLibraryA, 
HMODULE, WINAPI, (LPCSTR lpLibFileName))
{
	HMODULE moduleAddress = _LoadLibraryA(lpLibFileName);

	if (moduleAddress)
		CallLoadLibraryACallbacks(lpLibFileName, moduleAddress);

	return moduleAddress;
}

AUTOHOOK_ABSOLUTEADDR(_LoadLibraryExW, LoadLibraryExW, 
HMODULE, WINAPI, (LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags))
{
	HMODULE moduleAddress = _LoadLibraryExW(lpLibFileName, hFile, dwFlags);

	if (moduleAddress)
		CallLoadLibraryWCallbacks(lpLibFileName, moduleAddress);

	return moduleAddress;
}

AUTOHOOK_ABSOLUTEADDR(_LoadLibraryW, LoadLibraryW, 
HMODULE, WINAPI, (LPCWSTR lpLibFileName))
{
	HMODULE moduleAddress = _LoadLibraryW(lpLibFileName);

	if (moduleAddress)
		CallLoadLibraryWCallbacks(lpLibFileName, moduleAddress);

	return moduleAddress;
}

void InstallInitialHooks()
{
	if (MH_Initialize() != MH_OK)
		spdlog::error("MH_Initialize (minhook initialization) failed");

	AUTOHOOK_DISPATCH()
}