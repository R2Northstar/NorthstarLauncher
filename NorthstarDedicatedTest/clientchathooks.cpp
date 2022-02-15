#include "pch.h"
#include "clientchathooks.h"
#include <rapidjson/document.h>
#include "squirrel.h"
#include "serverchathooks.h"
#include "localchatwriter.h"

typedef void(__fastcall* CHudChat__AddGameLineType)(void* self, const char* message, int fromPlayerId, bool isteam, bool isdead);
CHudChat__AddGameLineType CHudChat__AddGameLine;

struct ChatTags
{
	bool whisper;
	bool team;
	bool dead;
};

struct Chat
{
	std::string message;
	int senderIndex;
	bool isTeam;
	bool isDead;
	int type;
};
Chat currentChat;

static void CHudChat__AddGameLineHook(void* self, const char* message, int inboxId, bool isTeam, bool isDead)
{
	// This hook is called for each HUD, but we only want our logic to run once.
	if (!IsFirstHud(self))
	{
		return;
	}

	int senderId = inboxId & CUSTOM_MESSAGE_INDEX_MASK;
	bool isAnonymous = senderId == 0;
	bool isCustom = isAnonymous || (inboxId & CUSTOM_MESSAGE_INDEX_BIT);

	// Type is set to 0 for non-custom messages, custom messages have a type encoded as the first byte
	int type = 0;
	const char* payload = message;
	if (isCustom)
	{
		type = message[0];
		payload = message + 1;
	}

	currentChat.message = payload;
	currentChat.senderIndex = senderId - 1;
	currentChat.isTeam = isTeam;
	currentChat.isDead = isDead;
	currentChat.type = type;
	g_ClientSquirrelManager->ExecuteCode("CHudChat_ProcessMessageStartThread()");
}

// void NSChatWrite( int context, string str )
static SQRESULT SQ_ChatWrite(void* sqvm)
{
	int context = ClientSq_getinteger(sqvm, 1);
	const char* str = ClientSq_getstring(sqvm, 2);

	LocalChatWriter((LocalChatWriter::Context)context).Write(str);
	return SQRESULT_NOTNULL;
}

// void NSChatWriteRaw( int context, string str )
static SQRESULT SQ_ChatWriteRaw(void* sqvm)
{
	int context = ClientSq_getinteger(sqvm, 1);
	const char* str = ClientSq_getstring(sqvm, 2);

	LocalChatWriter((LocalChatWriter::Context)context).InsertText(str);
	return SQRESULT_NOTNULL;
}

// void NSChatWriteLine( int context, string str )
static SQRESULT SQ_ChatWriteLine(void* sqvm)
{
	int context = ClientSq_getinteger(sqvm, 1);
	const char* str = ClientSq_getstring(sqvm, 2);

	LocalChatWriter((LocalChatWriter::Context)context).WriteLine(str);
	return SQRESULT_NOTNULL;
}

// string NSChatGetCurrentMessage()
static SQRESULT SQ_ChatGetCurrentMessage(void* sqvm)
{
	ClientSq_pushstring(sqvm, currentChat.message.c_str(), -1);
	return SQRESULT_NOTNULL;
}

// int NSChatGetCurrentPlayer()
static SQRESULT SQ_ChatGetCurrentPlayer(void* sqvm)
{
	ClientSq_pushinteger(sqvm, currentChat.senderIndex);
	return SQRESULT_NOTNULL;
}

// bool NSChatGetIsTeam()
static SQRESULT SQ_ChatGetIsTeam(void* sqvm)
{
	ClientSq_pushbool(sqvm, currentChat.isTeam);
	return SQRESULT_NOTNULL;
}

// bool NSChatGetIsDead()
static SQRESULT SQ_ChatGetIsDead(void* sqvm)
{
	ClientSq_pushbool(sqvm, currentChat.isDead);
	return SQRESULT_NOTNULL;
}

// int NSChatGetCurrentType()
static SQRESULT SQ_ChatGetCurrentType(void* sqvm)
{
	ClientSq_pushinteger(sqvm, currentChat.type);
	return SQRESULT_NOTNULL;
}

void InitialiseClientChatHooks(HMODULE baseAddress)
{
	HookEnabler hook;
	ENABLER_CREATEHOOK(hook, (char*)baseAddress + 0x22E580, &CHudChat__AddGameLineHook, reinterpret_cast<LPVOID*>(&CHudChat__AddGameLine));

	g_ClientSquirrelManager->AddFuncRegistration("void", "NSChatWrite", "int context, string text", "", SQ_ChatWrite);
	g_ClientSquirrelManager->AddFuncRegistration("void", "NSChatWriteRaw", "int context, string text", "", SQ_ChatWriteRaw);
	g_ClientSquirrelManager->AddFuncRegistration("void", "NSChatWriteLine", "int context, string text", "", SQ_ChatWriteLine);

	g_ClientSquirrelManager->AddFuncRegistration("string", "NSChatGetCurrentMessage", "", "", SQ_ChatGetCurrentMessage);
	g_ClientSquirrelManager->AddFuncRegistration("int", "NSChatGetCurrentPlayer", "", "", SQ_ChatGetCurrentPlayer);
	g_ClientSquirrelManager->AddFuncRegistration("bool", "NSChatGetIsTeam", "", "", SQ_ChatGetIsTeam);
	g_ClientSquirrelManager->AddFuncRegistration("bool", "NSChatGetIsDead", "", "", SQ_ChatGetIsDead);
	g_ClientSquirrelManager->AddFuncRegistration("int", "NSChatGetCurrentType", "", "", SQ_ChatGetCurrentType);
}
