#include "pch.h"
#include "convar.h"
#include "sourceconsole.h"
#include "sourceinterface.h"
#include "concommand.h"
#include "printcommand.h"

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

void SourceConsoleSink::sink_it_(const spdlog::details::log_msg& msg)
{
	if (!(*g_pSourceGameConsole)->m_bInitialized)
		return;

	spdlog::memory_buf_t formatted;
	spdlog::sinks::base_sink<std::mutex>::formatter_->format(msg, formatted);

	// if we arent using colour, we should exit early
	if (!g_bSpdLog_UseAnsiClr)
	{
		// dont use ColorPrint since we arent using colour
		(*g_pSourceGameConsole)->m_pConsole->m_pConsolePanel->Print(fmt::to_string(formatted).c_str());
		return;
	}

	// get the message "tags" (bits of string surrounded with [])
	// try to get the colour for each "tag"
	// print to the console with colours

	// get message string
	std::string str = fmt::to_string(formatted);
	std::map<int, SourceColor> colStrings = {};
	SourceColor baseCol = m_LogColours[msg.level];
	int idx = 0;
	while (true)
	{
		idx = str.find('[', idx);
		if (idx == std::string::npos)
			break;
		int startIdx = idx;
		std::string buf = "";
		// find the end of the tag
		while (++idx < str.length() && str[idx] != ']')
		{
			buf += str[idx];
		}
		// if we didn't find a closing tag, break
		if (str[idx] != ']')
			break;
		// we found a closing tag, make the colours
		if (m_tags.find(buf) == m_tags.end())
		{
			// if its an unknown tag (no colour), then just use base colour
			colStrings.insert(std::make_pair(startIdx + 1, baseCol));
		}
		else
		{
			colStrings.insert(std::make_pair(startIdx + 1, m_tags[buf]));
		}

		colStrings.insert(std::make_pair(idx, baseCol));
	}

	// iterate through our coloured strings and ColorPrint them in order
	int lastIdx = 0;
	SourceColor lastCol = baseCol;
	for (auto it = colStrings.begin(); it != colStrings.end(); ++it)
	{
		int curIdx = it->first;
		SourceColor curCol = it->second;

		std::string sub = str.substr(lastIdx, curIdx - lastIdx);
		(*g_pSourceGameConsole)->m_pConsole->m_pConsolePanel->ColorPrint(lastCol, sub.c_str());

		lastIdx = curIdx;
		lastCol = curCol;
	}
	// write the last bit of the string
	std::string sub = str.substr(lastIdx, str.length() - lastIdx);
	(*g_pSourceGameConsole)->m_pConsole->m_pConsolePanel->ColorPrint(lastCol, sub.c_str());
}

void SourceConsoleSink::flush_() {}

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
	OnCommandSubmittedHook.Dispatch((*g_pSourceGameConsole)->m_pConsole->m_vtable->OnCommandSubmitted);

	auto consoleLogger = std::make_shared<SourceConsoleSink>();
	if (g_bSpdLog_UseAnsiClr)
		consoleLogger->set_pattern("%v"); // no need to include the level in the game console, the text colour signifies it anyway
	else
		consoleLogger->set_pattern("[%l] %v"); // no colour, so we should show the level for colourblind people
	spdlog::default_logger()->sinks().push_back(consoleLogger);
}

ON_DLL_LOAD_CLIENT_RELIESON("client.dll", SourceConsole, ConCommand, (CModule module))
{
	g_pSourceGameConsole = new SourceInterface<CGameConsole>("client.dll", "GameConsole004");

	RegisterConCommand("toggleconsole", ConCommand_toggleconsole, "Show/hide the console.", FCVAR_DONTRECORD);
	RegisterConCommand("showconsole", ConCommand_showconsole, "Show the console.", FCVAR_DONTRECORD);
	RegisterConCommand("hideconsole", ConCommand_hideconsole, "Hide the console.", FCVAR_DONTRECORD);
}
