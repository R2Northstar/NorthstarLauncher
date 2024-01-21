#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "loader.h"
#include <string>
#include <system_error>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <shlwapi.h>

namespace fs = std::filesystem;

void LibraryLoadError(DWORD dwMessageId, const wchar_t* libName, const wchar_t* location)
{
	char text[8192];
	std::string message = std::system_category().message(dwMessageId);

	sprintf_s(
		text,
		"Failed to load the %ls at \"%ls\" (%lu):\n\n%hs\n\nMake sure you followed the Northstar installation instructions carefully "
		"before reaching out for help.",
		libName,
		location,
		dwMessageId,
		message.c_str());

	if (dwMessageId == 126 && std::filesystem::exists(location))
	{
		sprintf_s(
			text,
			"%s\n\nThe file at the specified location DOES exist, so this error indicates that one of its *dependencies* failed to be "
			"found.\n\nTry the following steps:\n1. Install Visual C++ 2022 Redistributable: "
			"https://aka.ms/vs/17/release/vc_redist.x64.exe\n2. Repair game files",
			text);
	}
	else if (!fs::exists("Titanfall2.exe") && (fs::exists("..\\Titanfall2.exe") || fs::exists("..\\..\\Titanfall2.exe")))
	{
		auto curDir = std::filesystem::current_path().filename().string();
		auto aboveDir = std::filesystem::current_path().parent_path().filename().string();
		sprintf_s(
			text,
			"%s\n\nWe detected that in your case you have extracted the files into a *subdirectory* of your Titanfall 2 "
			"installation.\nPlease move all the files and folders from current folder (\"%s\") into the Titanfall 2 installation directory "
			"just above (\"%s\").\n\nPlease try out the above steps by yourself before reaching out to the community for support.",
			text,
			curDir.c_str(),
			aboveDir.c_str());
	}
	else if (!fs::exists("Titanfall2.exe"))
	{
		sprintf_s(
			text,
			"%s\n\nRemember: you need to unpack the contents of this archive into your Titanfall 2 game installation directory, not just "
			"to any random folder.",
			text);
	}
	else if (fs::exists("Titanfall2.exe"))
	{
		sprintf_s(
			text,
			"%s\n\nTitanfall2.exe has been found in the current directory: is the game installation corrupted or did you not unpack all "
			"Northstar files here?",
			text);
	}
	MessageBoxA(GetForegroundWindow(), text, "Northstar Launcher Error", 0);
}

bool ShouldLoadNorthstar(bool default_case)
{
	bool loadNorthstar;

	if (default_case)
		loadNorthstar = strstr(GetCommandLineA(), "-nonorthstardll") == NULL;
	else
		loadNorthstar = strstr(GetCommandLineA(), "-northstar") != NULL;

	if (loadNorthstar)
		return loadNorthstar;

	auto runNorthstarFile = std::ifstream("run_northstar.txt");
	if (runNorthstarFile)
	{
		std::stringstream runNorthstarFileBuffer;
		runNorthstarFileBuffer << runNorthstarFile.rdbuf();
		runNorthstarFile.close();
		if (runNorthstarFileBuffer.str().starts_with("0"))
			loadNorthstar = false;
		else
			loadNorthstar = true;
	}
	return loadNorthstar;
}

bool LoadNorthstar()
{
	FARPROC Hook_Init = nullptr;
	{
		std::string strProfile = "R2Northstar";
		char* clachar = strstr(GetCommandLineA(), "-profile=");
		if (clachar)
		{
			std::string cla = std::string(clachar);
			if (strncmp(cla.substr(9, 1).c_str(), "\"", 1))
			{
				int space = cla.find(" ");
				std::string dirname = cla.substr(9, space - 9);
				std::cout << "[*] Found profile in command line arguments: " << dirname << std::endl;
				strProfile = dirname.c_str();
			}
			else
			{
				std::string quote = "\"";
				int quote1 = cla.find(quote);
				int quote2 = (cla.substr(quote1 + 1)).find(quote);
				std::string dirname = cla.substr(quote1 + 1, quote2);
				std::cout << "[*] Found profile in command line arguments: " << dirname << std::endl;
				strProfile = dirname;
			}
		}
		else
		{
			std::cout << "[*] Profile was not found in command line arguments. Using default: R2Northstar" << std::endl;
			strProfile = "R2Northstar";
		}

		wchar_t buffer[8192];

		// Check if "Northstar.dll" exists in profile directory, if it doesnt fall back to root
		swprintf_s(buffer, L"%s\\%s\\Northstar.dll", exePath, std::wstring(strProfile.begin(), strProfile.end()).c_str());

		if (!fs::exists(fs::path(buffer)))
			swprintf_s(buffer, L"%s\\Northstar.dll", exePath);

		std::wcout << L"[*] Using: " << buffer << std::endl;

		HMODULE hHookModule = LoadLibraryExW(buffer, 0, 8u);
		if (hHookModule)
			Hook_Init = GetProcAddress(hHookModule, "InitialiseNorthstar");
		if (!hHookModule || Hook_Init == nullptr)
		{
			LibraryLoadError(GetLastError(), L"Northstar.dll", buffer);
			return false;
		}
	}
	((bool (*)())Hook_Init)();

	return true;
}

bool GetExePathWide(wchar_t* dest, DWORD destSize)
{
	if (!dest)
		return NULL;
	if (destSize < MAX_PATH)
		return NULL;

	DWORD length = GetModuleFileNameW(NULL, dest, destSize);
	return length && PathRemoveFileSpecW(dest);
}
