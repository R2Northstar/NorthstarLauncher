#include "pch.h"
#include "serverchathooks.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include "serverauthentication.h"
#include "squirrel.h"
#include "miscserverscript.h"

class CServerGameDLL;
class CBasePlayer;

class CRecipientFilter
{
	char unknown[58];
};

CServerGameDLL* gServer;

typedef void(__fastcall* CServerGameDLL__OnReceivedSayTextMessageType)(
	CServerGameDLL* self, unsigned int senderPlayerIndex, const char* text, int channelId);
CServerGameDLL__OnReceivedSayTextMessageType CServerGameDLL__OnReceivedSayTextMessage;
CServerGameDLL__OnReceivedSayTextMessageType CServerGameDLL__OnReceivedSayTextMessageHookBase;

typedef CBasePlayer*(__fastcall* UTIL_PlayerByIndexType)(int playerIndex);
UTIL_PlayerByIndexType UTIL_PlayerByIndex;

typedef void(__fastcall* CRecipientFilter__ConstructType)(CRecipientFilter* self);
CRecipientFilter__ConstructType CRecipientFilter__Construct;

typedef void(__fastcall* CRecipientFilter__DestructType)(CRecipientFilter* self);
CRecipientFilter__DestructType CRecipientFilter__Destruct;

typedef void(__fastcall* CRecipientFilter__AddAllPlayersType)(CRecipientFilter* self);
CRecipientFilter__AddAllPlayersType CRecipientFilter__AddAllPlayers;

typedef void(__fastcall* CRecipientFilter__AddRecipientType)(CRecipientFilter* self, const CBasePlayer* player);
CRecipientFilter__AddRecipientType CRecipientFilter__AddRecipient;

typedef void(__fastcall* CRecipientFilter__MakeReliableType)(CRecipientFilter* self);
CRecipientFilter__MakeReliableType CRecipientFilter__MakeReliable;

typedef void(__fastcall* UserMessageBeginType)(CRecipientFilter* filter, const char* messagename);
UserMessageBeginType UserMessageBegin;

typedef void(__fastcall* MessageEndType)();
MessageEndType MessageEnd;

typedef void(__fastcall* MessageWriteByteType)(int iValue);
MessageWriteByteType MessageWriteByte;

typedef void(__fastcall* MessageWriteStringType)(const char* sz);
MessageWriteStringType MessageWriteString;

typedef void(__fastcall* MessageWriteBoolType)(bool bValue);
MessageWriteBoolType MessageWriteBool;

static std::string currentMessage;
static int currentPlayerId;
static int currentIsTeam;
static bool shouldBlock;

static SQRESULT setMessage(void* sqvm)
{
	currentMessage = ServerSq_getstring(sqvm, 1);
	currentPlayerId = ServerSq_getinteger(sqvm, 2);
	currentIsTeam = ServerSq_getinteger(sqvm, 3);
	shouldBlock = ServerSq_getbool(sqvm, 4);

	if (!shouldBlock)
	{
		ChatSendMessage(currentPlayerId, currentMessage.c_str(), currentIsTeam);
	}

	return SQRESULT_NOTNULL;
}
static SQRESULT getMessageServer(void* sqvm)
{
	ServerSq_pushstring(sqvm, currentMessage.c_str(), -1);
	return SQRESULT_NOTNULL;
}
static SQRESULT getPlayerServer(void* sqvm)
{
	ServerSq_pushinteger(sqvm, currentPlayerId);
	return SQRESULT_NOTNULL;
}
static SQRESULT getTeamServer(void* sqvm)
{
	ServerSq_pushinteger(sqvm, currentIsTeam);
	return SQRESULT_NOTNULL;
}

bool isSkippingHook = false;

static void
CServerGameDLL__OnReceivedSayTextMessageHook(CServerGameDLL* self, unsigned int senderPlayerIndex, const char* text, bool isTeam)
{
	// MiniHook doesn't allow calling the base function outside of anywhere but the hook function.
	// To allow bypassing the hook, isSkippingHook can be set.
	if (isSkippingHook)
	{
		isSkippingHook = false;
		CServerGameDLL__OnReceivedSayTextMessageHookBase(self, senderPlayerIndex, text, isTeam);
	}

	void* sender = GetPlayerByIndex(senderPlayerIndex - 1);

	// check chat ratelimits
	if (!g_ServerAuthenticationManager->CheckPlayerChatRatelimit(sender))
	{
		return;
	}

	currentMessage = text;
	currentPlayerId = senderPlayerIndex - 1;
	currentIsTeam = isTeam;
	shouldBlock = false;
	g_ServerSquirrelManager->ExecuteCode("CServerGameDLL_ProcessMessageStartThread()");
}

void ChatSendMessage(unsigned int playerId, const char* text, bool isteam)
{
	isSkippingHook = true;
	CServerGameDLL__OnReceivedSayTextMessage(
		gServer,
		// Ensure the first bit isn't set, since this indicates a custom message
		(playerId + 1) & CUSTOM_MESSAGE_INDEX_MASK, text, isteam);
}

void ChatBroadcastMessage(int fromPlayerId, int toPlayerId, const char* text, bool isTeam, bool isDead, CustomMessageType messageType)
{
	CBasePlayer* toPlayer = NULL;
	if (toPlayerId >= 0)
	{
		toPlayer = UTIL_PlayerByIndex(toPlayerId + 1);
		if (toPlayer == NULL)
			return;
	}

	// Build a new string where the first byte is the message type
	char sendText[256];
	sendText[0] = (char)messageType;
	strncpy(sendText + 1, text, 255);
	sendText[255] = 0;

	// Anonymous custom messages use playerId=0, non-anonymous ones use a player index with the first bit set
	unsigned int fromPlayerIndex = fromPlayerId < 0 ? 0 : ((toPlayerId + 1) | CUSTOM_MESSAGE_INDEX_BIT);

	CRecipientFilter filter;
	CRecipientFilter__Construct(&filter);
	if (toPlayer == NULL)
	{
		CRecipientFilter__AddAllPlayers(&filter);
	}
	else
	{
		CRecipientFilter__AddRecipient(&filter, toPlayer);
	}
	CRecipientFilter__MakeReliable(&filter);

	UserMessageBegin(&filter, "SayText");
	MessageWriteByte(fromPlayerIndex);
	MessageWriteString(sendText);
	MessageWriteBool(isTeam);
	MessageWriteBool(isDead);
	MessageEnd();

	CRecipientFilter__Destruct(&filter);
}

SQRESULT SQ_SendMessage(void* sqvm)
{
	int playerId = ServerSq_getinteger(sqvm, 1);
	const char* text = ServerSq_getstring(sqvm, 2);
	bool isTeam = ServerSq_getbool(sqvm, 3);

	ChatSendMessage(playerId, text, isTeam);

	return SQRESULT_NULL;
}

SQRESULT SQ_BroadcastMessage(void* sqvm)
{
	int fromPlayerId = ServerSq_getinteger(sqvm, 1);
	int toPlayerId = ServerSq_getinteger(sqvm, 2);
	const char* text = ServerSq_getstring(sqvm, 3);
	bool isTeam = ServerSq_getbool(sqvm, 4);
	bool isDead = ServerSq_getbool(sqvm, 5);
	int messageType = ServerSq_getinteger(sqvm, 6);

	if (messageType < 1 || messageType >= (int)CustomMessageType::ENUM_MAX)
	{
		ServerSq_pusherror(sqvm, fmt::format("Invalid message type {}", messageType).c_str());
		return SQRESULT_ERROR;
	}

	ChatBroadcastMessage(fromPlayerId, toPlayerId, text, isTeam, isDead, (CustomMessageType)messageType);

	return SQRESULT_NULL;
}

void InitialiseServerChatHooks_Engine(HMODULE baseAddress) { gServer = (CServerGameDLL*)((char*)baseAddress + 0x13F0AA98); }

void InitialiseServerChatHooks_Server(HMODULE baseAddress)
{
	CServerGameDLL__OnReceivedSayTextMessage = (CServerGameDLL__OnReceivedSayTextMessageType)((char*)baseAddress + 0x1595C0);
	UTIL_PlayerByIndex = (UTIL_PlayerByIndexType)((char*)baseAddress + 0x26AA10);
	CRecipientFilter__Construct = (CRecipientFilter__ConstructType)((char*)baseAddress + 0x1E9440);
	CRecipientFilter__Destruct = (CRecipientFilter__DestructType)((char*)baseAddress + 0x1E9700);
	CRecipientFilter__AddAllPlayers = (CRecipientFilter__AddAllPlayersType)((char*)baseAddress + 0x1E9940);
	CRecipientFilter__AddRecipient = (CRecipientFilter__AddRecipientType)((char*)baseAddress + 0x1E9b30);
	CRecipientFilter__MakeReliable = (CRecipientFilter__MakeReliableType)((char*)baseAddress + 0x1EA4E0);

	UserMessageBegin = (UserMessageBeginType)((char*)baseAddress + 0x15C520);
	MessageEnd = (MessageEndType)((char*)baseAddress + 0x158880);
	MessageWriteByte = (MessageWriteByteType)((char*)baseAddress + 0x158A90);
	MessageWriteString = (MessageWriteStringType)((char*)baseAddress + 0x158D00);
	MessageWriteBool = (MessageWriteBoolType)((char*)baseAddress + 0x158A00);

	HookEnabler hook;
	ENABLER_CREATEHOOK(
		hook, CServerGameDLL__OnReceivedSayTextMessage, &CServerGameDLL__OnReceivedSayTextMessageHook,
		reinterpret_cast<LPVOID*>(&CServerGameDLL__OnReceivedSayTextMessageHookBase));

	// Chat intercept functions
	g_ServerSquirrelManager->AddFuncRegistration(
		"void", "NSSetMessage", "string message, int playerId, int channelId, bool shouldBlock", "", setMessage);
	g_ServerSquirrelManager->AddFuncRegistration("string", "NSChatGetCurrentMessage", "", "", getMessageServer);
	g_ServerSquirrelManager->AddFuncRegistration("int", "NSChatGetCurrentPlayer", "", "", getPlayerServer);
	g_ServerSquirrelManager->AddFuncRegistration("bool", "NSChatGetIsTeam", "", "", getTeamServer);

	// Chat sending functions
	g_ServerSquirrelManager->AddFuncRegistration("void", "NSSendMessage", "int playerId, string text, bool isTeam", "", SQ_SendMessage);
	g_ServerSquirrelManager->AddFuncRegistration(
		"void", "NSBroadcastMessage", "int fromPlayerId, int toPlayerId, string text, bool isTeam, bool isDead, int messageType", "",
		SQ_BroadcastMessage);
}
