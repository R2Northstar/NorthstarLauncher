#include "core/convar/convar.h"
#include "sourceconsole.h"
#include "core/sourceinterface.h"
#include "core/convar/concommand.h"
#include "util/printcommands.h"

SourceInterface<CGameConsole>* g_pSourceGameConsole;

void ConCommand_toggleconsole(const CCommand& arg)
{
	if ((*g_pSourceGameConsole)->IsConsoleVisible())
		(*g_pSourceGameConsole)->Hide();
	else
		(*g_pSourceGameConsole)->Activate();
}

void ConCommand_showconsole(const CCommand& arg)
{
	(*g_pSourceGameConsole)->Activate();
}

void ConCommand_hideconsole(const CCommand& arg)
{
	(*g_pSourceGameConsole)->Hide();
}

// clang-format off
HOOK(OnCommandSubmittedHook, OnCommandSubmitted, 
void, __fastcall, (CConsoleDialog* consoleDialog, const char* pCommand))
// clang-format on
{
	consoleDialog->m_pConsolePanel->Print("] ");
	consoleDialog->m_pConsolePanel->Print(pCommand);
	consoleDialog->m_pConsolePanel->Print("\n");

	TryPrintCvarHelpForCommand(pCommand);

	OnCommandSubmitted(consoleDialog, pCommand);
}

// called from sourceinterface.cpp in client createinterface hooks, on GameClientExports001
void InitialiseConsoleOnInterfaceCreation()
{
	(*g_pSourceGameConsole)->Initialize();
	// hook OnCommandSubmitted so we print inputted commands
	OnCommandSubmittedHook.Dispatch((LPVOID)(*g_pSourceGameConsole)->m_pConsole->m_vtable->OnCommandSubmitted);
}

ON_DLL_LOAD_CLIENT_RELIESON("client.dll", SourceConsole, ConCommand, (CModule module))
{
	g_pSourceGameConsole = new SourceInterface<CGameConsole>("client.dll", "GameConsole004");

	RegisterConCommand("toggleconsole", ConCommand_toggleconsole, "Show/hide the console.", FCVAR_DONTRECORD);
	RegisterConCommand("showconsole", ConCommand_showconsole, "Show the console.", FCVAR_DONTRECORD);
	RegisterConCommand("hideconsole", ConCommand_hideconsole, "Hide the console.", FCVAR_DONTRECORD);
}
