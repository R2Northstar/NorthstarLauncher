#include "pch.h"
#include "hooks.h"
#include "hookutils.h"
#include "sigscanning.h"
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

typedef LPSTR (*GetCommandLineAType)();
LPSTR GetCommandLineAHook();

typedef HMODULE (*LoadLibraryExAType)(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
HMODULE LoadLibraryExAHook(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);

typedef HMODULE (*LoadLibraryAType)(LPCSTR lpLibFileName);
HMODULE LoadLibraryAHook(LPCSTR lpLibFileName);

typedef HMODULE (*LoadLibraryExWType)(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
HMODULE LoadLibraryExWHook(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);

typedef HMODULE (*LoadLibraryWType)(LPCWSTR lpLibFileName);
HMODULE LoadLibraryWHook(LPCWSTR lpLibFileName);

GetCommandLineAType GetCommandLineAOriginal;
LoadLibraryExAType LoadLibraryExAOriginal;
LoadLibraryAType LoadLibraryAOriginal;
LoadLibraryExWType LoadLibraryExWOriginal;
LoadLibraryWType LoadLibraryWOriginal;

void InstallInitialHooks()
{
	if (MH_Initialize() != MH_OK)
		spdlog::error("MH_Initialize (minhook initialization) failed");

	HookEnabler hook;
	ENABLER_CREATEHOOK(hook, &GetCommandLineA, &GetCommandLineAHook, reinterpret_cast<LPVOID*>(&GetCommandLineAOriginal));
	ENABLER_CREATEHOOK(hook, &LoadLibraryExA, &LoadLibraryExAHook, reinterpret_cast<LPVOID*>(&LoadLibraryExAOriginal));
	ENABLER_CREATEHOOK(hook, &LoadLibraryA, &LoadLibraryAHook, reinterpret_cast<LPVOID*>(&LoadLibraryAOriginal));
	ENABLER_CREATEHOOK(hook, &LoadLibraryExW, &LoadLibraryExWHook, reinterpret_cast<LPVOID*>(&LoadLibraryExWOriginal));
	ENABLER_CREATEHOOK(hook, &LoadLibraryW, &LoadLibraryWHook, reinterpret_cast<LPVOID*>(&LoadLibraryWOriginal));
}

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

LPSTR GetCommandLineAHook()
{
	static char* cmdlineModified;
	static char* cmdlineOrg;

	if (cmdlineOrg == nullptr || cmdlineModified == nullptr)
	{
		cmdlineOrg = GetCommandLineAOriginal();
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

// for whatever reason, just declaring and initialising the vector at file scope crashes on debug builds
// but this works, idk sucks but just how it is
std::vector<DllLoadCallback>& GetDllLoadCallbacks()
{
	static std::vector<DllLoadCallback> vec = std::vector<DllLoadCallback>();
	return vec;
}

void AddDllLoadCallback(std::string dll, DllLoadCallbackFuncType callback, std::string tag, std::string reliesOn)
{
	DllLoadCallback& callbackStruct = GetDllLoadCallbacks().emplace_back(); // <-- crashes here

	callbackStruct.dll = dll;
	callbackStruct.callback = callback;
	callbackStruct.tag = tag;
	callbackStruct.reliesOn = reliesOn;
	callbackStruct.called = false;
}

void AddDllLoadCallbackForDedicatedServer(
	std::string dll, DllLoadCallbackFuncType callback, std::string tag, std::string reliesOn)
{
	if (!IsDedicated())
		return;

	AddDllLoadCallback(dll, callback, tag, reliesOn);
}

void AddDllLoadCallbackForClient(std::string dll, DllLoadCallbackFuncType callback, std::string tag, std::string reliesOn)
{
	if (IsDedicated())
		return;

	AddDllLoadCallback(dll, callback, tag, reliesOn);
}

std::vector<std::string> calledTags;

void CallLoadLibraryACallbacks(LPCSTR lpLibFileName, HMODULE moduleAddress)
{
	while (true)
	{
		bool doneCalling = true;

		for (auto& callbackStruct : GetDllLoadCallbacks())
		{
			if (!callbackStruct.called && fs::path(lpLibFileName).filename() == fs::path(callbackStruct.dll).filename())
			{
				if (callbackStruct.reliesOn != "" &&
					std::find(calledTags.begin(), calledTags.end(), callbackStruct.reliesOn) == calledTags.end())
				{
					doneCalling = false;
					continue;
				}

				callbackStruct.callback(moduleAddress);
				calledTags.push_back(callbackStruct.tag);
				callbackStruct.called = true;
			}
		}

		if (doneCalling)
			break;
	}
}

void CallLoadLibraryWCallbacks(LPCWSTR lpLibFileName, HMODULE moduleAddress)
{
	while (true)
	{
		bool doneCalling = true;

		for (auto& callbackStruct : GetDllLoadCallbacks())
		{
			if (!callbackStruct.called && fs::path(lpLibFileName).filename() == fs::path(callbackStruct.dll).filename())
			{
				if (callbackStruct.reliesOn != "" &&
					std::find(calledTags.begin(), calledTags.end(), callbackStruct.reliesOn) == calledTags.end())
				{
					doneCalling = false;
					continue;
				}

				callbackStruct.callback(moduleAddress);
				calledTags.push_back(callbackStruct.tag);
				callbackStruct.called = true;
			}
		}

		if (doneCalling)
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

HMODULE LoadLibraryExAHook(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
	HMODULE moduleAddress = LoadLibraryExAOriginal(lpLibFileName, hFile, dwFlags);

	if (moduleAddress)
	{
		CallLoadLibraryACallbacks(lpLibFileName, moduleAddress);
	}

	return moduleAddress;
}

HMODULE LoadLibraryAHook(LPCSTR lpLibFileName)
{
	HMODULE moduleAddress = LoadLibraryAOriginal(lpLibFileName);

	if (moduleAddress)
	{
		CallLoadLibraryACallbacks(lpLibFileName, moduleAddress);
	}

	return moduleAddress;
}

HMODULE LoadLibraryExWHook(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
	HMODULE moduleAddress = LoadLibraryExWOriginal(lpLibFileName, hFile, dwFlags);

	if (moduleAddress)
	{
		CallLoadLibraryWCallbacks(lpLibFileName, moduleAddress);
	}

	return moduleAddress;
}

HMODULE LoadLibraryWHook(LPCWSTR lpLibFileName)
{
	HMODULE moduleAddress = LoadLibraryWOriginal(lpLibFileName);

	if (moduleAddress)
	{
		CallLoadLibraryWCallbacks(lpLibFileName, moduleAddress);
	}

	return moduleAddress;
}