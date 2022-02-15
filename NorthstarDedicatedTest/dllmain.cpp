#include "pch.h"
#include "hooks.h"
#include "main.h"
#include "squirrel.h"
#include "dedicated.h"
#include "dedicatedmaterialsystem.h"
#include "sourceconsole.h"
#include "logging.h"
#include "concommand.h"
#include "modmanager.h"
#include "filesystem.h"
#include "serverauthentication.h"
#include "scriptmodmenu.h"
#include "scriptserverbrowser.h"
#include "keyvalues.h"
#include "masterserver.h"
#include "gameutils.h"
#include "chatcommand.h"
#include "modlocalisation.h"
#include "playlist.h"
#include "securitypatches.h"
#include "miscserverscript.h"
#include "clientauthhooks.h"
#include "latencyflex.h"
#include "scriptbrowserhooks.h"
#include "scriptmainmenupromos.h"
#include "miscclientfixes.h"
#include "miscserverfixes.h"
#include "rpakfilesystem.h"
#include "bansystem.h"
#include "memalloc.h"
#include "maxplayers.h"
#include "languagehooks.h"
#include "audio.h"
#include "buildainfile.h"
#include "configurables.h"
#include <string.h>
#include "pch.h"
#include "urihandler.h"
#include <shellapi.h>
#include <iostream>
#include <fstream>
#include <WinUser.h>

bool initialised = false;

void writeToRegistry(std::string exepath)
{
	::ShowWindow(::GetConsoleWindow(), SW_HIDE);

	HKEY runKey;
	DWORD dontcare;

	if (RegCreateKeyEx(HKEY_CLASSES_ROOT, L"northstar\\shell\\open\\command", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &runKey, &dontcare) !=
		ERROR_SUCCESS)
	{
		return;
	}
	else
	{
		std::string command = exepath + " %1";
		std::wstring wide_string = std::wstring(command.begin(), command.end());
		LPCTSTR data = wide_string.c_str();
		LPCTSTR flag = L"";
		LPCTSTR name = L"Northstar Launcher";
		RegSetKeyValueW(HKEY_CLASSES_ROOT, L"northstar\\shell\\open\\command", L"", REG_SZ, (LPBYTE)data, wcslen(data) * sizeof(TCHAR));
		RegSetKeyValueW(HKEY_CLASSES_ROOT, L"northstar", L"URL Protocol", REG_SZ, (LPBYTE)flag, wcslen(flag) * sizeof(TCHAR));
		RegSetKeyValueW(HKEY_CLASSES_ROOT, L"northstar", L"", REG_SZ, (LPBYTE)name, wcslen(name) * sizeof(TCHAR));
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}

void WaitForDebugger(HMODULE baseAddress)
{
	// earlier waitfordebugger call than is in vanilla, just so we can debug stuff a little easier
	if (CommandLine()->CheckParm("-waitfordebugger"))
	{
		spdlog::info("waiting for debugger...");

		while (!IsDebuggerPresent())
			Sleep(100);
	}
}

bool InitialiseNorthstar()
{

	InstallInitialHooks();

	wchar_t buffer[_MAX_PATH];
	GetModuleFileNameW(NULL, buffer, _MAX_PATH); // Get full executable path
	std::wstring w = std::wstring(buffer);
	std::string exepath = std::string(w.begin(), w.end()); // Convert from wstring to string

	if (strstr(GetCommandLineA(), "-addurihandler"))
	{
		writeToRegistry(exepath);
		exit(0);
	}

	HKEY subKey = nullptr;
	LONG result = RegOpenKeyEx(HKEY_CLASSES_ROOT, L"northstar", 0, KEY_READ, &subKey);
	std::ofstream file;
	if (result != ERROR_SUCCESS && !strstr(GetCommandLineA(), "-nopopupurihandler"))
	{
		int msgboxID = MessageBox(
			NULL, (LPCWSTR)L"Would you like to allow Northstar to automatically open invite links when you click on them?\n\nClick YES to allow\nClick NO to disable forever\nClick CANCEL to ask again next time", (LPCWSTR)L"Northstar Launcher",
			MB_ICONQUESTION | MB_YESNOCANCEL | MB_DEFBUTTON1 | MB_APPLMODAL
		);
		switch (msgboxID)
		{
		case IDYES:
			ShellExecute(
				NULL, L"runas", w.c_str(), L" -addurihandler",
				NULL, // default dir
				SW_SHOWNORMAL
			);
			break;
		case IDNO:
			file.open("ns_startup_args.txt", std::ios::app | std::ios::out);
			file.write(" -nopopupurihandler", 20);
			file.close();
			break;
		case IDCANCEL:
			break;
		default:
			break;
		}
		
	}

	if (initialised)
	{
		// spdlog::warn("Called InitialiseNorthstar more than once!"); // it's actually 100% fine for that to happen
		return false;
	}

	initialised = true;

	parseConfigurables();

	std::string cla = GetCommandLineA();
	std::string URIProtocolName = "northstar://";
	int uriOffset = cla.find(URIProtocolName);
	if (uriOffset != -1)
	{
		std::string message = cla.substr(uriOffset, cla.length() - uriOffset - 1); // -1 to remove a trailing slash
		parseURI(message);
	}
	else
	{
		spdlog::info("================================");
		spdlog::info("================================");
		spdlog::info("================================");
		spdlog::info("================================");
		spdlog::info("No URI found");
	}

	SetEnvironmentVariableA("OPENSSL_ia32cap", "~0x200000200000000");
	curl_global_init_mem(CURL_GLOBAL_DEFAULT, _malloc_base, _free_base, _realloc_base, _strdup_base, _calloc_base);

	InitialiseLogging();
	CreateLogFiles();
	InitialiseInterfaceCreationHooks();

	AddDllLoadCallback("tier0.dll", InitialiseTier0GameUtilFunctions);
	AddDllLoadCallback("engine.dll", WaitForDebugger);
	AddDllLoadCallback("engine.dll", InitialiseEngineGameUtilFunctions);
	AddDllLoadCallback("server.dll", InitialiseServerGameUtilFunctions);

	// dedi patches
	{
		AddDllLoadCallback("tier0.dll", InitialiseDedicatedOrigin);
		AddDllLoadCallback("engine.dll", InitialiseDedicated);
		AddDllLoadCallback("server.dll", InitialiseDedicatedServerGameDLL);
		AddDllLoadCallback("materialsystem_dx11.dll", InitialiseDedicatedMaterialSystem);
		// this fucking sucks, but seemingly we somehow load after rtech_game???? unsure how, but because of this we have to apply patches
		// here, not on rtech_game load
		AddDllLoadCallback("engine.dll", InitialiseDedicatedRtechGame);
	}

	AddDllLoadCallback("engine.dll", InitialiseConVars);
	AddDllLoadCallback("engine.dll", InitialiseConCommands);

	// client-exclusive patches
	{
		AddDllLoadCallback("tier0.dll", InitialiseTier0LanguageHooks);
		AddDllLoadCallback("engine.dll", InitialiseClientEngineSecurityPatches);
		AddDllLoadCallback("client.dll", InitialiseClientSquirrel);
		AddDllLoadCallback("client.dll", InitialiseSourceConsole);
		AddDllLoadCallback("engine.dll", InitialiseChatCommands);
		AddDllLoadCallback("client.dll", InitialiseScriptModMenu);
		AddDllLoadCallback("client.dll", InitialiseScriptServerBrowser);
		AddDllLoadCallback("localize.dll", InitialiseModLocalisation);
		AddDllLoadCallback("engine.dll", InitialiseClientAuthHooks);
		AddDllLoadCallback("client.dll", InitialiseLatencyFleX);
		AddDllLoadCallback("engine.dll", InitialiseScriptExternalBrowserHooks);
		AddDllLoadCallback("client.dll", InitialiseScriptMainMenuPromos);
		AddDllLoadCallback("client.dll", InitialiseMiscClientFixes);
		AddDllLoadCallback("client.dll", InitialiseClientPrintHooks);
		AddDllLoadCallback("client.dll", InitialiseURIStuff);
	}

	AddDllLoadCallback("engine.dll", InitialiseEngineSpewFuncHooks);
	AddDllLoadCallback("server.dll", InitialiseServerSquirrel);
	AddDllLoadCallback("engine.dll", InitialiseBanSystem);
	AddDllLoadCallback("engine.dll", InitialiseServerAuthentication);
	AddDllLoadCallback("server.dll", InitialiseServerAuthenticationServerDLL);
	AddDllLoadCallback("engine.dll", InitialiseSharedMasterServer);
	AddDllLoadCallback("server.dll", InitialiseMiscServerScriptCommand);
	AddDllLoadCallback("server.dll", InitialiseMiscServerFixes);
	AddDllLoadCallback("server.dll", InitialiseBuildAINFileHooks);

	AddDllLoadCallback("engine.dll", InitialisePlaylistHooks);

	AddDllLoadCallback("filesystem_stdio.dll", InitialiseFilesystem);
	AddDllLoadCallback("engine.dll", InitialiseEngineRpakFilesystem);
	AddDllLoadCallback("engine.dll", InitialiseKeyValues);

	// maxplayers increase
	AddDllLoadCallback("engine.dll", InitialiseMaxPlayersOverride_Engine);
	AddDllLoadCallback("client.dll", InitialiseMaxPlayersOverride_Client);
	AddDllLoadCallback("server.dll", InitialiseMaxPlayersOverride_Server);

	// audio hooks
	AddDllLoadCallback("client.dll", InitialiseMilesAudioHooks);

	// mod manager after everything else
	AddDllLoadCallback("engine.dll", InitialiseModManager);

	// run callbacks for any libraries that are already loaded by now
	CallAllPendingDLLLoadCallbacks();

	return true;
}