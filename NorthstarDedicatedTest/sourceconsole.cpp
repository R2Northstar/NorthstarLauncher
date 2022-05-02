#include "pch.h"
#include "convar.h"
#include "sourceconsole.h"
#include "sourceinterface.h"
#include "concommand.h"
#include "hookutils.h"

SourceInterface<CGameConsole>* g_SourceGameConsole;
ConVar* ns_log_printfs;

void ConCommand_toggleconsole(const CCommand& arg)
{
	if ((*g_SourceGameConsole)->IsConsoleVisible())
		(*g_SourceGameConsole)->Hide();
	else
		(*g_SourceGameConsole)->Activate();
}

typedef void (*OnCommandSubmittedType)(CConsoleDialog* consoleDialog, const char* pCommand);
OnCommandSubmittedType onCommandSubmittedOriginal;
void OnCommandSubmittedHook(CConsoleDialog* consoleDialog, const char* pCommand)
{
	consoleDialog->m_pConsolePanel->Print("] ");
	consoleDialog->m_pConsolePanel->Print(pCommand);
	consoleDialog->m_pConsolePanel->Print("\n");

	// todo: call the help command in the future

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

void InitialiseSourceConsole(HMODULE baseAddress)
{
	g_SourceGameConsole = new SourceInterface<CGameConsole>("client.dll", "GameConsole004");
	RegisterConCommand("toggleconsole", ConCommand_toggleconsole, "toggles the console", FCVAR_DONTRECORD);
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

// show all convars and commands

__int64 Q_snprintf(char* a1, signed __int64 a2, const char* a3, ...)
{
	__int64 result; // rax
	va_list ArgList; // [rsp+58h] [rbp+20h] BYREF

	va_start(ArgList, a3);
	if (a2 <= 0)
		return 0i64;
	result = vsnprintf(a1, a2, a3, ArgList);
	if ((int)result < 0i64 || (int)result >= a2)
	{
		result = a2 - 1;
		a1[a2 - 1] = 0;
	}
	if (ns_log_printfs->GetInt() != 0)
		spdlog::info(a1);
	return result;
}

int __cdecl printCCommand(CCommand& args)
{
	int argc = args.ArgC();
	std::string echoStr;

	for (int i = 1; i < argc; i++)
	{
		echoStr.append(args[i]);
		echoStr.append(" ");
	}
	spdlog::info(echoStr);
	return 0;
}

typedef __int64(__fastcall ConVar_PrintDescriptionType(const struct ConCommandBase near* a1));
ConVar_PrintDescriptionType* ConVar_PrintDescriptionOriginal;
bool bIsPrintingConvarDesc;

char* lastCvarDesc;

__int64 __fastcall ConVar_PrintDescription(const struct ConCommandBase near* a1)
{
	bIsPrintingConvarDesc = true;
	__int64 ret = ConVar_PrintDescriptionOriginal(a1);
	bIsPrintingConvarDesc = false;
	std::string fmt2 = std::string(lastCvarDesc);

	if (ret && ((char*)ret)[0])
	{
		std::string fmt = std::string((char*)ret);

		spdlog::info("{} - {}", fmt2, fmt);
	}
	else
	{
		spdlog::info("{}", fmt2);
	}
	return ret;
}
char const* stristr(char const* pStr, char const* pSearch)
{

	if (!pStr || !pSearch)
		return 0;

	char const* pLetter = pStr;

	// Check the entire string
	while (*pLetter != 0)
	{
		// Skip over non-matches
		if (tolower((unsigned char)*pLetter) == tolower((unsigned char)*pSearch))
		{
			// Check for match
			char const* pMatch = pLetter + 1;
			char const* pTest = pSearch + 1;
			while (*pTest != 0)
			{
				// We've run off the end; don't bother.
				if (*pMatch == 0)
					return 0;

				if (tolower((unsigned char)*pMatch) != tolower((unsigned char)*pTest))
					break;

				++pMatch;
				++pTest;
			}

			// Found a match!
			if (*pTest == 0)
				return pLetter;
		}

		++pLetter;
	}

	return 0;
}
void Find(const CCommand& args)
{

	const char* search;
	const ConCommandBase* var;

	if (args.ArgC() != 2)
	{
		spdlog::info("Usage:  find <string>");
		return;
	}

	// Get substring to find
	search = args[1];

	// Loop through vars and print out findings

	for (auto& map : g_pCVar->DumpToMap())
	{
		ConCommandBase* var = g_pCVar->FindCommandBase(map.first.c_str());
		if (!var)
			continue;
		if (!stristr(var->m_pszName, search) && !stristr(var->GetHelpText(), search))
			continue;

		ConVar_PrintDescription((const ConCommandBase*)var);
	}
}

static char* StripTabsAndReturns(const char* inbuffer, char* outbuffer, int outbufferSize)
{
	char* out = outbuffer;
	const char* i = inbuffer;
	char* o = out;

	out[0] = 0;

	while (*i && o - out < outbufferSize - 1)
	{
		if (*i == '\n' || *i == '\r' || *i == '\t')
		{
			*o++ = ' ';
			i++;
			continue;
		}
		if (*i == '\"')
		{
			*o++ = '\'';
			i++;
			continue;
		}

		*o++ = *i++;
	}

	*o = '\0';

	return out;
}

struct ConVarFlags_t
{
	int bit;
	const char* desc;
	const char* shortdesc;
};
#define CONVARFLAG(x, y)                                                                                                                   \
	{                                                                                                                                      \
		FCVAR_##x, #x, #y                                                                                                                  \
	}

static ConVarFlags_t g_ConVarFlags[] = {
	//	CONVARFLAG( UNREGISTERED, "u" ),
	CONVARFLAG(ARCHIVE, "a"),
	CONVARFLAG(SPONLY, "sp"),
	CONVARFLAG(GAMEDLL, "sv"),
	CONVARFLAG(CHEAT, "cheat"),
	CONVARFLAG(USERINFO, "user"),
	CONVARFLAG(NOTIFY, "nf"),
	CONVARFLAG(PROTECTED, "prot"),
	CONVARFLAG(PRINTABLEONLY, "print"),
	CONVARFLAG(UNLOGGED, "log"),
	CONVARFLAG(NEVER_AS_STRING, "numeric"),
	CONVARFLAG(REPLICATED, "rep"),
	CONVARFLAG(DEMO, "demo"),
	CONVARFLAG(DONTRECORD, "norecord"),
	CONVARFLAG(SERVER_CAN_EXECUTE, "server_can_execute"),
	CONVARFLAG(CLIENTCMD_CAN_EXECUTE, "clientcmd_can_execute"),
	CONVARFLAG(CLIENTDLL, "cl"),
};
#define COPY_ALL_CHARACTERS -1

char* Q_strncat(char* pDest, const char* pSrc, size_t destBufferSize, int max_chars_to_copy)
{
	size_t charstocopy = (size_t)0;

	size_t len = strlen(pDest);
	size_t srclen = strlen(pSrc);
	if (max_chars_to_copy <= COPY_ALL_CHARACTERS)
	{
		charstocopy = srclen;
	}
	else
	{
		charstocopy = (size_t)(std::min)(max_chars_to_copy, (int)srclen);
	}

	if (len + charstocopy >= destBufferSize)
	{
		charstocopy = destBufferSize - len - 1;
	}

	if (!charstocopy)
	{
		return pDest;
	}

	char* pOut = strncat(pDest, pSrc, charstocopy);
	pOut[destBufferSize - 1] = 0;
	return pOut;
}

char* Q_strncatConvar(char* pDest, const char* pSrc, size_t destBufferSize, int max_chars_to_copy)
{
	size_t charstocopy = (size_t)0;

	size_t len = strlen(pDest);
	size_t srclen = strlen(pSrc);
	if (max_chars_to_copy <= COPY_ALL_CHARACTERS)
	{
		charstocopy = srclen;
	}
	else
	{
		charstocopy = (size_t)(std::min)(max_chars_to_copy, (int)srclen);
	}

	if (len + charstocopy >= destBufferSize)
	{
		charstocopy = destBufferSize - len - 1;
	}

	if (!charstocopy)
	{
		return pDest;
	}

	char* pOut = strncat(pDest, pSrc, charstocopy);
	pOut[destBufferSize - 1] = 0;
	if (bIsPrintingConvarDesc)
		lastCvarDesc = pOut;
	return pOut;
}

void ConMsg(const char* pMsgFormat, ...)
{
	char pTempBuffer[5020];

	va_list args;
	va_start(args, pMsgFormat);
	_vsnprintf(pTempBuffer, 5020, pMsgFormat, args);
	va_end(args);
	spdlog::info(pTempBuffer);
}

bool strcasecmp(const char* a1, const char* a2)
{
	int i = 0;
	while (!a1[i])
	{
		if (tolower(a1[i]) != tolower(a2[i]))
			return false;
		i++;
	}
	return true;
}

void CvarList(const CCommand& args)
{
	const ConCommandBase* var; // Temporary Pointer to cvars
	int iArgs; // Argument count
	const char* partial = NULL; // Partial cvar to search for...
								// E.eg
	int ipLen = 0; // Length of the partial cvar

	bool bLogging = false;
	// Are we logging?
	iArgs = args.ArgC(); // Get count

	// Print usage?
	if (iArgs == 2 && !strcasecmp(args[1], "?"))
	{
		spdlog::info("cvarlist: [ partial ]");
		return;
	}
	partial = args[1];
	ipLen = strlen(partial);

	// Banner
	spdlog::info("cvar list\n--------------");

	std::vector<ConCommandBase*> sorted;
	// Loop through cvars...
	for (auto& map : g_pCVar->DumpToMap())
	{
		ConCommandBase* var = g_pCVar->FindCommandBase(map.first.c_str());
		if (!var)
			continue;
		bool print = false;

		if (partial) // Partial string searching?
		{
			if (!_strnicmp(var->m_pszName, partial, ipLen))
			{
				print = true;
			}
		}
		else
		{
			print = true;
		}

		if (!print)
			continue;

		sorted.push_back(var);
	}
	std::sort(
		sorted.begin(),
		sorted.end(),
		[](const ConCommandBase* a, const ConCommandBase* b) -> bool
		{ return std::string(a->m_pszName).compare(std::string(b->m_pszName)) < 0; });

	for (auto& base : sorted)
	{
		if (base->IsCommand())
		{
			const ConCommand* cmd = reinterpret_cast<const ConCommand*>(base);

			char tempbuff[128];
			ConMsg(
				"%-40s : %-8s : %-16s : %s",
				cmd->m_pszName,
				"cmd",
				"",
				StripTabsAndReturns(cmd->GetHelpText(), tempbuff, sizeof(tempbuff)));
		}
		else
		{
			ConVar* var = reinterpret_cast<ConVar*>(base);

			char flagstr[128];
			char csvflagstr[1024];

			flagstr[0] = 0;
			csvflagstr[0] = 0;

			int c = ARRAYSIZE(g_ConVarFlags);
			for (int i = 0; i < c; ++i)
			{
				char f[32];
				char csvf[64];

				ConVarFlags_t& entry = g_ConVarFlags[i];
				if (var->IsFlagSet(var, entry.bit))
				{
					Q_snprintf(f, sizeof(f), ", %s", entry.shortdesc);

					Q_strncat(flagstr, f, sizeof(flagstr), COPY_ALL_CHARACTERS);

					Q_snprintf(csvf, sizeof(csvf), "\"%s\",", entry.desc);
				}
				else
				{
					Q_snprintf(csvf, sizeof(csvf), ",");
				}

				Q_strncat(csvflagstr, csvf, sizeof(csvflagstr), COPY_ALL_CHARACTERS);
			}

			char valstr[32];
			char tempbuff[128];

			// Clean up integers
			if (var->GetInt() == (int)var->GetFloat())
			{
				Q_snprintf(valstr, sizeof(valstr), "%-8i", var->GetInt());
			}
			else
			{
				Q_snprintf(valstr, sizeof(valstr), "%-8.3f", var->GetFloat());
			}

			// Print to console
			ConMsg(
				"%-40s : %-8s : %-16s : %s",
				var->GetBaseName(),
				valstr,
				flagstr,
				StripTabsAndReturns(var->GetHelpText(), tempbuff, sizeof(tempbuff)));
		}
	}

	// Show total and syntax help...
	if (partial && partial[0])
	{
		ConMsg("--------------\n%3i convars/concommands for [%s]", sorted.size(), partial);
	}
	else
	{
		ConMsg("--------------\n%3i total convars/concommands", sorted.size());
	}
}

typedef __int64 __fastcall Host_InitType(char a1);
Host_InitType* Host_InitOrig;

__int64 __fastcall Host_InitHook(char a1)
{
	auto ret = Host_InitOrig(a1);
	for (auto& map : g_pCVar->DumpToMap())
	{
		ConCommandBase* base = g_pCVar->FindCommandBase(map.first.c_str());
		if (!base)
			continue;
		base->RemoveFlags(FCVAR_DEVELOPMENTONLY);
		base->RemoveFlags(FCVAR_HIDDEN);
	}
	return ret;
}

void SourceConsoleSink::flush_() {}

void InitializeConCommandPrintFunctions(HMODULE baseAddress)
{

	HookEnabler hook;
	ns_log_printfs = new ConVar("ns_log_printfs", "0", FCVAR_NONE, "");

	ENABLER_CREATEHOOK(hook, (char*)baseAddress + 0x414990, &Q_snprintf, reinterpret_cast<LPVOID*>(NULL));
	ENABLER_CREATEHOOK(hook, (char*)baseAddress + 0x414BC0, &Q_strncatConvar, reinterpret_cast<LPVOID*>(NULL));
	ENABLER_CREATEHOOK(hook, (char*)baseAddress + 0x123680, &printCCommand, reinterpret_cast<LPVOID*>(NULL));
	ENABLER_CREATEHOOK(
		hook, (char*)baseAddress + 0x416E20, &ConVar_PrintDescription, reinterpret_cast<LPVOID*>(&ConVar_PrintDescriptionOriginal));
	ENABLER_CREATEHOOK(hook, (char*)baseAddress + 0x1567E0, &Host_InitHook, reinterpret_cast<LPVOID*>(&Host_InitOrig));
	RegisterConCommand("find", Find, "Find concommands with the specified string in their name / help text.", 0);
	RegisterConCommand("cvarlist", CvarList, "Show the list of convars/concommands.", 0);
}
