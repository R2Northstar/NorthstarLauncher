#include "pch.h"
#include "loader.h"
#include "include/MinHook.h"
#include <string>
#include <system_error>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <iostream>

typedef void (*InitializeSplashScreenType)(const char* loadPath);

void LibraryLoadError(DWORD dwMessageId, const wchar_t* libName, const wchar_t* location)
{
	char text[4096];
	std::string message = std::system_category().message(dwMessageId);
	sprintf_s(text, "Failed to load the %ls at \"%ls\" (%lu):\n\n%hs", libName, location, dwMessageId, message.c_str());
	if (dwMessageId == 126 && std::filesystem::exists(location))
	{
		sprintf_s(
			text,
			"%s\n\nThe file at the specified location DOES exist, so this error indicates that one of its *dependencies* failed to be "
			"found.",
			text);
	}
	MessageBoxA(GetForegroundWindow(), text, "Northstar Wsock32 Proxy Error", 0);
}

bool ShouldLoadNorthstar()
{
	bool loadNorthstar = strstr(GetCommandLineA(), "-northstar");

	if (loadNorthstar)
		return loadNorthstar;

	auto runNorthstarFile = std::ifstream("run_northstar.txt");
	if (runNorthstarFile)
	{
		std::stringstream runNorthstarFileBuffer;
		runNorthstarFileBuffer << runNorthstarFile.rdbuf();
		runNorthstarFile.close();
		if (!runNorthstarFileBuffer.str()._Starts_with("0"))
			loadNorthstar = true;
	}
	return loadNorthstar;
}

bool LoadNorthstar()
{
	FARPROC Hook_Init = nullptr;
	{
		swprintf_s(buffer1, L"%s\\Northstar.dll", exePath);
		auto hHookModule = LoadLibraryExW(buffer1, 0, LOAD_WITH_ALTERED_SEARCH_PATH);
		if (hHookModule)
			Hook_Init = GetProcAddress(hHookModule, "InitialiseNorthstar");
		if (!hHookModule || Hook_Init == nullptr)
		{
			LibraryLoadError(GetLastError(), L"Northstar.dll", buffer1);
			return false;
		}
	}

	((bool (*)())Hook_Init)();
	return true;
}

typedef int (*LauncherMainType)(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow);
LauncherMainType LauncherMainOriginal;

int LauncherMainHook(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	if (ShouldLoadNorthstar())
		LoadNorthstar();
	return LauncherMainOriginal(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}

bool ProvisionNorthstar()
{
	if (!ShouldLoadNorthstar())
		return true;

	bool dedicated = strstr(GetCommandLineA(), "-dedicated");
	bool nosplash = strstr(GetCommandLineA(), "-nosplash");
	bool showConsole = strstr(GetCommandLineA(), "-showconsole");

	if (!dedicated && !nosplash)
	{
		char* clachar = strstr(GetCommandLineA(), "-altsplash=");
		std::string altSplash = "";
		if (clachar)
		{
			std::string cla = clachar;
			if (strncmp(cla.substr(12, 1).c_str(), "\"", 1))
			{
				cla = cla.substr(12);
				int space = cla.find("\"");
				altSplash = cla.substr(0, space);
			}
			else
			{
				std::string quote = "\"";
				int quote1 = cla.find(quote);
				int quote2 = (cla.substr(quote1 + 1)).find(quote);
				altSplash = cla.substr(quote1 + 1, quote2);
			}
		}
		auto splashDLL = LoadLibraryExW(L"splash.dll", 0, LOAD_WITH_ALTERED_SEARCH_PATH);
		if (!splashDLL)
		{
			MessageBoxA(
				GetForegroundWindow(),
				"Failed to load splash.dll!",
				"Northstar Wsock32 Proxy Error",
				0);
			return false;
		}
		auto initSplash = (InitializeSplashScreenType)GetProcAddress(splashDLL, "InitializeSplashScreen");
		if (!initSplash)
		{
			MessageBoxA(GetForegroundWindow(), "Failed to load splash screen!", "Northstar Wsock32 Proxy Error", 0);
			return false;
		}
		else
		{
			initSplash(altSplash.c_str());
			DisableProcessWindowsGhosting();
		}
	}

	if (!nosplash && !showConsole)
	{
		ShowWindow(GetConsoleWindow(), SW_HIDE);
	}

	if (MH_Initialize() != MH_OK)
	{
		MessageBoxA(
			GetForegroundWindow(), "MH_Initialize failed\nThe game cannot continue and has to exit.", "Northstar Wsock32 Proxy Error", 0);
		return false;
	}

	auto launcherHandle = GetModuleHandleA("launcher.dll");
	if (!launcherHandle)
	{
		MessageBoxA(
			GetForegroundWindow(),
			"Launcher isn't loaded yet.\nThe game cannot continue and has to exit.",
			"Northstar Wsock32 Proxy Error",
			0);
		return false;
	}

	LPVOID pTarget = GetProcAddress(launcherHandle, "LauncherMain");
	if (MH_CreateHook(pTarget, &LauncherMainHook, reinterpret_cast<LPVOID*>(&LauncherMainOriginal)) != MH_OK ||
		MH_EnableHook(pTarget) != MH_OK)
		MessageBoxA(GetForegroundWindow(), "Hook creation failed for function LauncherMain.", "Northstar Wsock32 Proxy Error", 0);

	return true;
}
