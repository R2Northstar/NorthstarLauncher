#include "pch.h"
#include "hooks.h"
#include "convar.h"
#include "sourceconsole.h"
#include "sourceinterface.h"
#include "concommand.h"
#include "hookutils.h"

SourceInterface<CGameConsole>* g_SourceGameConsole;

void ConCommand_toggleconsole(const CCommand& arg)
{
	if ((*g_SourceGameConsole)->IsConsoleVisible())
		(*g_SourceGameConsole)->Hide();
	else
		(*g_SourceGameConsole)->Activate();
}

void ConCommand_showconsole(const CCommand& arg)
{
	(*g_SourceGameConsole)->Activate();
}

void ConCommand_hideconsole(const CCommand& arg)
{
	(*g_SourceGameConsole)->Hide();
}

void PrintCommandHelpDialogue(const ConCommandBase* command, const char* name)
{
	if (!command)
	{
		spdlog::info("unknown command {}", name);
		return;
	}

	// build string for flags if not FCVAR_NONE
	std::string flagString;
	if (command->GetFlags() != FCVAR_NONE)
	{
		flagString = "( ";

		for (auto& flagPair : g_PrintCommandFlags)
		{
			if (command->GetFlags() & flagPair.first)
			{
				flagString += flagPair.second;
				flagString += " ";
			}
		}

		flagString += ") ";
	}

	// temp because command->IsCommand does not currently work
	ConVar* cvar = g_pCVar->FindVar(command->m_pszName);
	if (cvar)
		spdlog::info("\"{}\" = \"{}\" {}- {}", cvar->GetBaseName(), cvar->GetString(), flagString, cvar->GetHelpText());
	else
		spdlog::info("\"{}\" {} - {}", command->m_pszName, flagString, command->GetHelpText());
}

void ConCommand_help(const CCommand& arg) 
{
	if (arg.ArgC() < 2)
	{
		spdlog::info("Usage: help <cvarname>");
		return;
	}

	PrintCommandHelpDialogue(g_pCVar->FindCommandBase(arg.Arg(1)), arg.Arg(1));
}

void ConCommand_find(const CCommand& arg) 
{
	if (arg.ArgC() < 2)
	{
		spdlog::info("Usage: find <string> [<string>...]");
		return;
	}

	char pTempName[256];
	char pTempSearchTerm[256];

	for (auto& map : g_pCVar->DumpToMap())
	{
		bool bPrintCommand = true;
		for (int i = 0; i < arg.ArgC() - 1; i++)
		{
			// make lowercase to avoid case sensitivity
			strncpy(pTempName, map.second->m_pszName, sizeof(pTempName));
			strncpy(pTempSearchTerm, arg.Arg(i + 1), sizeof(pTempSearchTerm));

			for (int i = 0; pTempName[i]; i++)
				pTempName[i] = tolower(pTempName[i]);

			for (int i = 0; pTempSearchTerm[i]; i++)
				pTempSearchTerm[i] = tolower(pTempSearchTerm[i]);

			if (!strstr(pTempName, pTempSearchTerm))
			{
				bPrintCommand = false;
				break;
			}
		}

		if (bPrintCommand)
			PrintCommandHelpDialogue(map.second, map.second->m_pszName);
	}
}

typedef void (*OnCommandSubmittedType)(CConsoleDialog* consoleDialog, const char* pCommand);
OnCommandSubmittedType onCommandSubmittedOriginal;
void OnCommandSubmittedHook(CConsoleDialog* consoleDialog, const char* pCommand)
{
	consoleDialog->m_pConsolePanel->Print("] ");
	consoleDialog->m_pConsolePanel->Print(pCommand);
	consoleDialog->m_pConsolePanel->Print("\n");

	// try to display help text for this cvar
	{
		int pCommandLen = strlen(pCommand);
		char* pCvarStr = new char[pCommandLen];
		strcpy(pCvarStr, pCommand);

		// trim whitespace from right
		for (int i = pCommandLen - 1; i; i--)
		{
			if (isspace(pCvarStr[i]))
				pCvarStr[i] = '\0';
			else
				break;
		}

		// check if we're inputting a cvar, but not setting it at all
		ConVar* cvar = g_pCVar->FindVar(pCvarStr);
		if (cvar)
			PrintCommandHelpDialogue(&cvar->m_ConCommandBase, pCvarStr);

		delete[] pCvarStr;
	}

	onCommandSubmittedOriginal(consoleDialog, pCommand);
}

// called from sourceinterface.cpp in client createinterface hooks, on GameClientExports001
void InitialiseConsoleOnInterfaceCreation()
{
	(*g_SourceGameConsole)->Initialize();

	auto consoleLogger = std::make_shared<SourceConsoleSink>();
	consoleLogger->set_pattern("[%l] %v");

	spdlog::default_logger()->sinks().push_back(consoleLogger);

	// hook OnCommandSubmitted so we print inputted commands
	HookEnabler hook;
	ENABLER_CREATEHOOK(
		hook,
		(void*)((*g_SourceGameConsole)->m_pConsole->m_vtable->OnCommandSubmitted),
		&OnCommandSubmittedHook,
		reinterpret_cast<LPVOID*>(&onCommandSubmittedOriginal));
}

// logging stuff

SourceConsoleSink::SourceConsoleSink()
{
	logColours.emplace(spdlog::level::trace, SourceColor(0, 255, 255, 255));
	logColours.emplace(spdlog::level::debug, SourceColor(0, 255, 255, 255));
	logColours.emplace(spdlog::level::info, SourceColor(255, 255, 255, 255));
	logColours.emplace(spdlog::level::warn, SourceColor(255, 255, 0, 255));
	logColours.emplace(spdlog::level::err, SourceColor(255, 0, 0, 255));
	logColours.emplace(spdlog::level::critical, SourceColor(255, 0, 0, 255));
	logColours.emplace(spdlog::level::off, SourceColor(0, 0, 0, 0));
}

void SourceConsoleSink::sink_it_(const spdlog::details::log_msg& msg)
{
	if (!(*g_SourceGameConsole)->m_bInitialized)
		return;

	spdlog::memory_buf_t formatted;
	spdlog::sinks::base_sink<std::mutex>::formatter_->format(msg, formatted);
	(*g_SourceGameConsole)
		->m_pConsole->m_pConsolePanel->ColorPrint(logColours[msg.level], fmt::to_string(formatted).c_str()); // todo needs colour support
}

void SourceConsoleSink::flush_() {}

ON_DLL_LOAD_CLIENT_RELIESON("client.dll", SourceConsole, ConCommand, [](HMODULE baseAddress)
{
	g_SourceGameConsole = new SourceInterface<CGameConsole>("client.dll", "GameConsole004");
	RegisterConCommand("toggleconsole", ConCommand_toggleconsole, "Show/hide the console.", FCVAR_DONTRECORD);
	RegisterConCommand("showconsole", ConCommand_showconsole, "Show the console.", FCVAR_DONTRECORD);
	RegisterConCommand("hideconsole", ConCommand_hideconsole, "Hide the console.", FCVAR_DONTRECORD);
	RegisterConCommand("find", ConCommand_find, "Find concommands with the specified string in their name/help text.", FCVAR_NONE);

	// help is already a command, so we need to modify the preexisting command to use our func instead
	// and clear the flags also
	ConCommand* helpCommand = g_pCVar->FindCommand("help");
	helpCommand->m_nFlags = 0;
	helpCommand->m_pCommandCallback = ConCommand_help;
})