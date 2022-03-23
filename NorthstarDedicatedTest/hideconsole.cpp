#include "pch.h"
#include "hideconsole.h"
#include "dedicated.h"
#include "convar.h"

ConVar* Cvar_show_console_window;

void ChangeConsoleHiddenState()
{
	if (Cvar_show_console_window->GetBool())
	{
		ShowWindow(GetConsoleWindow(), SW_SHOWNOACTIVATE);
	}
	else
	{
		ShowWindow(GetConsoleWindow(), SW_HIDE);
	}
}

void InitialiseHiddenConsole(HMODULE baseAddress)
{
	if (!strstr(GetCommandLineA(), "-showconsole"))
	{
		ShowWindow(GetConsoleWindow(), SW_HIDE);
		Cvar_show_console_window = new ConVar("show_console_window", "0", FCVAR_NONE, "", false, 0, true, 1, ChangeConsoleHiddenState);
	}
	else
	{
		Cvar_show_console_window = new ConVar("show_console_window", "1", FCVAR_NONE, "", false, 0, true, 1, ChangeConsoleHiddenState);
	}
}
