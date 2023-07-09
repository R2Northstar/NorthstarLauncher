#include "logging.h"
#include "core/convar/convar.h"
#include "core/convar/concommand.h"
#include "config/profile.h"
#include "core/tier0.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_sinks.h"

#include <iomanip>
#include <sstream>
#include <shellapi.h>
#include <dedicated/dedicated.h>

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
// Purpose:
//-----------------------------------------------------------------------------
void SpdLog_Init(void)
{
	g_WinLogger = spdlog::stdout_logger_mt("win_console");
	spdlog::set_default_logger(g_WinLogger);
	g_WinLogger->set_level(spdlog::level::trace);

	if (g_bSpdLog_UseAnsiColor)
		g_WinLogger->set_pattern("%v\u001b[0m");
	else
		g_WinLogger->set_pattern("%v");
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void SpdLog_Shutdown(void)
{
	spdlog::shutdown();
}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void Console_Init(void)
{
	g_bSpdLog_UseAnsiColor = strstr(GetCommandLineA(), "-noansicolor") == NULL;

	// Always show console when we're a dedicated server
	bool bShow = strstr(GetCommandLineA(), "-wconsole") != NULL || IsDedicatedServer();

	if (bShow)
	{
		(void)AllocConsole();

		FILE* pDummy;
		freopen_s(&pDummy, "CONIN$", "r", stdin);
		freopen_s(&pDummy, "CONOUT$", "w", stdout);
		freopen_s(&pDummy, "CONOUT$", "w", stderr);

		if (g_bSpdLog_UseAnsiColor)
		{
			DWORD dwMode = 0;
			HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);

			GetConsoleMode(hOutput, &dwMode);
			dwMode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;

			if (!SetConsoleMode(hOutput, dwMode))
			{
				g_bSpdLog_UseAnsiColor = false;
			}
		}
	}
	else
	{
		// TODO [Fifty]: Only close console once gamewindow is created
		HWND hConsole = GetConsoleWindow();
		(void)FreeConsole();
		(void)PostMessageA(hConsole, WM_CLOSE, 0, 0);
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void Console_Shutdown(void) {}
