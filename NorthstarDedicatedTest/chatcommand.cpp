#include "pch.h"
#include "chatcommand.h"
#include "concommand.h"
#include "dedicated.h"
#include "clientchathooks.h"
#include "squirrel.h"

// note: isIngameChat is an int64 because the whole register the arg is stored in needs to be 0'd out to work
// if isIngameChat is false, we use network chat instead
typedef void(__fastcall* ClientSayTextType)(void* a1, const char* message, __int64 isIngameChat, bool isTeamChat);
ClientSayTextType ClientSayText;

const char* whisperReceiver;
const char* whisperMessage;

void ConCommand_say(const CCommand& args)
{
	if (args.ArgC() >= 2)
		ClientSayText(nullptr, args.ArgS(), true, false);
}

void ConCommand_say_team(const CCommand& args)
{
	if (args.ArgC() >= 2)
		ClientSayText(nullptr, args.ArgS(), true, true);
}

void ConCommand_log(const CCommand& args)
{
	if (args.ArgC() >= 2)
	{
		LocalChatWriteLine(LocalChatContext::Game, args.ArgS(), 128, AnonymousMessageType::Whisper);
	}
}

void InitialiseChatCommands(HMODULE baseAddress)
{
	if (IsDedicated())
		return;

	ClientSayText = (ClientSayTextType)((char*)baseAddress + 0x54780);
	RegisterConCommand("say", ConCommand_say, "Enters a message in public chat", FCVAR_CLIENTDLL);
	RegisterConCommand("say_team", ConCommand_say_team, "Enters a message in team chat", FCVAR_CLIENTDLL);
	RegisterConCommand("log", ConCommand_log, "Log a message to the local chat window", FCVAR_CLIENTDLL);
}