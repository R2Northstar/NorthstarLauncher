#include "logging.h"
#include "core/convar/convar.h"
#include "core/convar/concommand.h"
#include "config/profile.h"
#include "core/tier0.h"
#include "util/version.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"

#include <winternl.h>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <shellapi.h>
#include <dedicated/dedicated.h>
#include <util/utils.h>

AUTOHOOK_INIT()

//-----------------------------------------------------------------------------
// Purpose: Checks if install folder is writable, exits if it is not
//-----------------------------------------------------------------------------
void SpdLog_PreInit(void)
{
	// This is called before SpdLog_Init so we can't use any logging helpers

	// NOTE [Fifty]: Instead of checking every reason as to why we might not be able to write to a directory
	//               it is easier to just try to create a file
	FILE* pFile = fopen(".nstemp", "w");
	if (pFile)
	{
		fclose(pFile);
		remove(".nstemp");
		return;
	}

	// Show message box
	int iAction = MessageBoxA(
		NULL,
		"The current directory isn't writable!\n"
		"Please move the game into a writable directory to be able to continue\n\n"
		"Click \"OK\" to open the wiki in your browser",
		"Northstar Error",
		MB_ICONERROR | MB_OKCANCEL);

	// User chose to open the troubleshooting wiki page
	if (iAction == IDOK)
	{
		ShellExecuteA(
			NULL,
			NULL,
			"https://r2northstar.gitbook.io/r2northstar-wiki/installing-northstar/"
			"troubleshooting#cannot-write-log-file-when-using-northstar-on-ea-app",
			NULL,
			NULL,
			SW_NORMAL);
	}

	// Gracefully close
	TerminateProcess(GetCurrentProcess(), -1);
}

//-----------------------------------------------------------------------------
// Purpose: Initilases spdlog + windows console logger
//-----------------------------------------------------------------------------
void SpdLog_Init(void)
{
	g_WinLogger = spdlog::stdout_logger_mt("win_console");
	spdlog::set_default_logger(g_WinLogger);
	spdlog::set_level(spdlog::level::trace);

	// NOTE [Fifty]: This may be bad as it writes to disk for every message?
	//               Seems to fix logs not flushing properly, still needs testing
	spdlog::flush_on(spdlog::level::trace);

	if (g_bConsole_UseAnsiColor)
		g_WinLogger->set_pattern("%v\u001b[0m");
	else
		g_WinLogger->set_pattern("%v");
}

//-----------------------------------------------------------------------------
// Purpose: Creates all loggers we use
//-----------------------------------------------------------------------------
void SpdLog_CreateLoggers(void)
{
	g_bSpdLog_CreateLogFiles = strstr(GetCommandLineA(), "-nologfiles") == NULL;

	if (!g_bSpdLog_CreateLogFiles)
		return;

	time_t time = std::time(nullptr);
	tm currentTime = *std::localtime(&time);

	std::stringstream stream;
	stream << g_svLogDirectory << std::put_time(&currentTime, "\\nslog%Y-%m-%d %H-%M-%S.txt");

	auto logger = spdlog::basic_logger_mt("northstar", stream.str());
	logger->set_pattern("[%Y-%m-%d] [%H:%M:%S] [%l] %v");
}

//-----------------------------------------------------------------------------
// Purpose: Shutdowns spdlog
//-----------------------------------------------------------------------------
void SpdLog_Shutdown(void)
{
	spdlog::shutdown();
}
//-----------------------------------------------------------------------------
// Purpose: Initilazes the windows console
//-----------------------------------------------------------------------------
void Console_Init(void)
{
	g_bConsole_UseAnsiColor = strstr(GetCommandLineA(), "-noansicolor") == NULL;

	// Always show the console when starting-up.
	(void)AllocConsole();

	FILE* pDummy;
	freopen_s(&pDummy, "CONIN$", "r", stdin);
	freopen_s(&pDummy, "CONOUT$", "w", stdout);
	freopen_s(&pDummy, "CONOUT$", "w", stderr);

	if (g_bConsole_UseAnsiColor)
	{
		DWORD dwMode = 0;
		HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);

		GetConsoleMode(hOutput, &dwMode);
		dwMode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;

		if (!SetConsoleMode(hOutput, dwMode))
		{
			g_bConsole_UseAnsiColor = false;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Closes the console window if wanted
//-----------------------------------------------------------------------------
void Console_PostInit(void)
{
	// Hide the console if user wants to
	bool bHide = strstr(GetCommandLineA(), "-wconsole") == NULL && !IsDedicatedServer();

	if (!bHide)
		return;

	HWND hConsole = GetConsoleWindow();
	(void)FreeConsole();
	(void)PostMessageA(hConsole, WM_CLOSE, 0, 0);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void Console_Shutdown(void) {}

//-----------------------------------------------------------------------------
// Wine specific functions
typedef const char*(CDECL* wine_get_host_version_type)(const char**, const char**);
wine_get_host_version_type wine_get_host_version;

typedef const char*(CDECL* wine_get_build_id_type)(void);
wine_get_build_id_type wine_get_build_id;

//-----------------------------------------------------------------------------
// Not exported Winapi methods
typedef NTSTATUS(WINAPI* RtlGetVersion_type)(PRTL_OSVERSIONINFOW);
RtlGetVersion_type RtlGetVersion;

//-----------------------------------------------------------------------------
// Purpose: Print Operating System information
//-----------------------------------------------------------------------------
void Sys_PrintOSVer()
{
	DevMsg(eLog::NS, "NorthstarLauncher version: %s\n", version);
	DevMsg(eLog::NS, "Command line: %s\n", GetCommandLineA());
	DevMsg(eLog::NS, "Using profile: %s\n", GetNorthstarPrefix().c_str());

	// ntdll is always loaded
	HMODULE ntdll = GetModuleHandleA("ntdll.dll");
	if (!ntdll)
	{
		DevMsg(eLog::NS, "Operating System: Unknown\n");
		return;
	}

	wine_get_host_version = (wine_get_host_version_type)GetProcAddress(ntdll, "wine_get_host_version");
	if (wine_get_host_version)
	{
		// Load the rest of the functions we need
		wine_get_build_id = (wine_get_build_id_type)GetProcAddress(ntdll, "wine_get_build_id");

		const char* sysname;
		wine_get_host_version(&sysname, NULL);

		DevMsg(eLog::NS, "Operating System: %s (Wine)\n", sysname);
		DevMsg(eLog::NS, "Wine build: %s\n", wine_get_build_id());

		// STEAM_COMPAT_TOOL_PATHS is a colon separated lists of all compat tool paths used
		// The first one tends to be the Proton path itself
		// We extract the basename out of it to get the name used
		char* compatToolPtr = std::getenv("STEAM_COMPAT_TOOL_PATHS");
		if (compatToolPtr)
		{
			std::string_view compatToolPath(compatToolPtr);

			auto protonBasenameEnd = compatToolPath.find(":");
			if (protonBasenameEnd == std::string_view::npos)
				protonBasenameEnd = 0;
			auto protonBasenameStart = compatToolPath.rfind("/", protonBasenameEnd) + 1;
			if (protonBasenameStart == std::string_view::npos)
				protonBasenameStart = 0;

			DevMsg(eLog::NS, "Proton build: %s\n", compatToolPath.substr(protonBasenameStart, protonBasenameEnd - protonBasenameStart));
		}
	}
	else
	{
		// We are real Windows (hopefully)
		const char* win_ver = "Unknown";

		RTL_OSVERSIONINFOW osvi;
		osvi.dwOSVersionInfoSize = sizeof(osvi);

		RtlGetVersion = (RtlGetVersion_type)GetProcAddress(ntdll, "RtlGetVersion");
		if (RtlGetVersion && !RtlGetVersion(&osvi))
		{
			// Version reference table
			// https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-osversioninfoa#remarks
			DevMsg(eLog::NS, "Operating System: Windows (NT%i.%i)\n", osvi.dwMajorVersion, osvi.dwMinorVersion);
		}
		else
		{
			DevMsg(eLog::NS, "Operating System: Windows\n");
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
// clang-format off
AUTOHOOK(Respawn_CreateWindow, engine.dll + 0x1CD0E0, bool, __fastcall,
	(void* a1))
// clang-format on
{
	Console_PostInit();
	return Respawn_CreateWindow(a1);
}

//-----------------------------------------------------------------------------
ON_DLL_LOAD_CLIENT("engine.dll", CreateWindowLog, (CModule module))
{
	AUTOHOOK_DISPATCH()
}
