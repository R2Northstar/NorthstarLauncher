#include "pch.h"
#include "AntiRCE.h"
#include "cvar.h"

static bool initialized = false;

void OnGameFileAccess(const char* pathArg, bool readOnly)
{
	if (!initialized)
		return;
	static char curDirectory[MAX_PATH];
	static DWORD curDirectoryResult = GetCurrentDirectoryA(MAX_PATH, curDirectory);
	static int curDirectoryLen = strlen(curDirectory);

	static ConVar* ns_log_all_file_access = new ConVar(
		"ns_log_all_file_access",
#ifdef _DEBUG
		"1",
#else
		"0",
#endif
		FCVAR_GAMEDLL,
		"Log all file access from the game");

	if (!pathArg)
		return;

	// Resolve full path
	char path[MAX_PATH] = {};
	_fullpath(path, pathArg, MAX_PATH);

	if (ns_log_all_file_access->GetBool())
	{
		spdlog::info("File handle opened to \"{}\", thread ID: {}", path, (void*)GetCurrentThreadId());
	}

	int pathLen = strlen(path);

	const char* extension = strrchr(path, '.');
	if (extension)
	{
		if (!readOnly)
		{
			constexpr const char* EVIL_EXTENTIONS[] = {".dll", ".exe", ".bat", ".cmd", ".com"};
			for (auto evil : EVIL_EXTENTIONS)
			{
				if (!_stricmp(evil, extension))
					AntiRCE::EmergencyReport("Tried to access file \"" + std::string(path) + "\"");
			}
		}

		constexpr const char* SAFE_EXTENTIONS[] = {".ttf", ".fon", ".ttc"};
		for (auto safe : SAFE_EXTENTIONS)
			if (!_stricmp(safe, extension))
				return; // This file can be safely accessed
	}

	if (strstr(path, "\\respawn\\titanfall2\\"))
		return; // Perfectly fine, just the temporary data path

	bool isRelativePath;
	if (pathLen >= curDirectoryLen)
	{
		isRelativePath = true;
		for (int i = 0; i < curDirectoryLen; i++)
		{
			char cur = path[i];
			char target = curDirectory[i];

			// Normalize slashes
			if (target == '/')
				target = '\\';
			if (cur == '/')
				cur = '\\';

			if (tolower(cur) != tolower(target))
			{
				isRelativePath = false;
				break;
			}
		}
	}
	else
	{
		isRelativePath = false;
	}

	if (!isRelativePath)
	{
		AntiRCE::EmergencyReport("Tried to access file \"" + std::string(path) + "\"");
	}
}

// Its 3 am and idk what this function is but it get called whenever file opened sooo
KHOOK(
	FileSytem_OpenFileFuncIdk,
	("filesystem_stdio.dll", "48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 54 41 55 41 56 41 57 48 81 EC ? ? ? ? 8B 05"),
	void*,
	__fastcall,
	(char* fileName, char* openTypeStr, void* unk))
{

	OnGameFileAccess(fileName, openTypeStr[0] == 'r');

	return oFileSytem_OpenFileFuncIdk(fileName, openTypeStr, unk);
}

// NOTE: These hooks do not prevent RCE attacks from using these funtions, as an attacker could just disable these hooks
// (particularly easy if you get a ROP-chain going and use MH_RemoveHook lol)
//
// These hooks are only meant to be a barrier of entry for RCEs,
//	as well as to prevent any exploits that might utilize these functions from being able to do actual damage
void AntiRCE::EmergencyReport(std::string msg)
{
#ifdef NS_DEBUG // For easier debugging
	assert(false, "AntiRCE::EmergencyReport triggered");
#endif

	spdlog::critical("AntiRCE EMERGENCY REPORT: " + msg);

#ifndef NS_DEBUG // This gets annoying during testing

	// Beep an out-of-tune tri-tone for 300ms so user knows something is up
	Beep(494 * 2, 150);
	Beep(349 * 2, 150);

#endif

	// clang-format off
	MessageBoxA(NULL, (
			"AntiRCE EMERGENCY SHUTDOWN:\n" + msg + 
			"\n\nIt is HIGHLY reccommended that you notify northstar developers/staff of this issue (unless you are sure it's fine)."
		).c_str(), 
		"Northstar AntiRCE", 
		MB_ICONERROR
	);
	// clang-format on

	exit(EXITCODE_TERRIBLE);
}

bool AntiRCE::Init()
{
	spdlog::info("Initializing AntiRCE...");

	bool result = true;

	{ // Notify user if syscalls are enabled
		PROCESS_MITIGATION_SYSTEM_CALL_DISABLE_POLICY policyBuffer;
		bool result = GetProcessMitigationPolicy(GetCurrentProcess(), ProcessSystemCallDisablePolicy, &policyBuffer, sizeof(policyBuffer));

		if (!result || !policyBuffer.DisallowWin32kSystemCalls)
		{
			spdlog::warn("AntiRCE: Raw syscalls are permitted, this can pose a security risk.");
		}
	}

	initialized = true;
	return result;
}