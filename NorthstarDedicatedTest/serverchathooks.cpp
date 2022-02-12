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
CServerGameDLL__OnReceivedSayTextMessageType CServerGameDLL__OnReceivedSayTextMessageUnhooked;

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

static bool g_SkipSayTextHook = false;

static std::string currentMessage;
static int currentPlayerId;
static int currentChannelId;
static bool shouldBlock;
static bool isProcessed = true;

static SQRESULT setMessage(void* sqvm)
{
	currentMessage = ServerSq_getstring(sqvm, 1);
	currentPlayerId = ServerSq_getinteger(sqvm, 2);
	currentChannelId = ServerSq_getinteger(sqvm, 3);
	shouldBlock = ServerSq_getbool(sqvm, 4);
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
static SQRESULT getChannelServer(void* sqvm)
{
	ServerSq_pushinteger(sqvm, currentChannelId);
	return SQRESULT_NOTNULL;
}
static SQRESULT getShouldProcessMessage(void* sqvm)
{
	ServerSq_pushbool(sqvm, !isProcessed);
	return SQRESULT_NOTNULL;
}
static SQRESULT pushMessage(void* sqvm)
{
	currentMessage = ServerSq_getstring(sqvm, 1);
	currentPlayerId = ServerSq_getinteger(sqvm, 2);
	currentChannelId = ServerSq_getinteger(sqvm, 3);
	shouldBlock = ServerSq_getbool(sqvm, 4);
	isProcessed = true;
	return SQRESULT_NOTNULL;
}

static void
CServerGameDLL__OnReceivedSayTextMessageHook(CServerGameDLL* self, unsigned int senderPlayerIndex, const char* text, int channelId)
{
	// Set by ChatSendFromPlayer so it can bypass any checks
	if (g_SkipSayTextHook)
	{
		g_SkipSayTextHook = false;
		CServerGameDLL__OnReceivedSayTextMessageUnhooked(self, senderPlayerIndex, text, channelId);
		return;
	}

	void* sender = GetPlayerByIndex(senderPlayerIndex - 1);

	// check chat ratelimits
	if (!g_ServerAuthenticationManager->CheckPlayerChatRatelimit(sender))
	{
		return;
	}

	bool shouldDoChathooks = strstr(GetCommandLineA(), "-enablechathooks");
	if (shouldDoChathooks)
	{

		currentMessage = text;
		currentPlayerId = senderPlayerIndex - 1; // Stupid fix cause of index offsets
		currentChannelId = channelId;
		shouldBlock = false;
		isProcessed = false;

		g_ServerSquirrelManager->ExecuteCode("CServerGameDLL_ProcessMessageStartThread()");
		if (!shouldBlock && currentPlayerId + 1 == senderPlayerIndex) // stop player id spoofing from server
		{
			CServerGameDLL__OnReceivedSayTextMessage(self, currentPlayerId + 1, currentMessage.c_str(), currentChannelId);
		}
	}
	else
	{
		CServerGameDLL__OnReceivedSayTextMessage(self, senderPlayerIndex, text, channelId);
	}
}

ChatRichText::ChatRichText()
{
	m_document.SetObject();
	m_document.AddMember("t", rapidjson::Value(rapidjson::kArrayType), m_document.GetAllocator());
}

void ChatRichText::ToString(rapidjson::StringBuffer& buffer) const
{
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	m_document.Accept(writer);
}

void ChatRichText::InsertLocalized(const char* str)
{
	rapidjson::Value obj(rapidjson::kObjectType);
	obj.AddMember("i", "l", m_document.GetAllocator());
	obj.AddMember("s", std::string(str), m_document.GetAllocator());
	m_document["t"].PushBack(std::move(obj), m_document.GetAllocator());
}

void ChatRichText::InsertText(const char* str)
{
	rapidjson::Value obj(rapidjson::kObjectType);
	obj.AddMember("i", "t", m_document.GetAllocator());
	obj.AddMember("s", std::string(str), m_document.GetAllocator());
	m_document["t"].PushBack(std::move(obj), m_document.GetAllocator());
}

void ChatRichText::InsertColor(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha)
{
	unsigned int color = (red << 24) + (green << 16) + (blue << 8) + alpha;

	rapidjson::Value obj(rapidjson::kObjectType);
	obj.AddMember("i", "c", m_document.GetAllocator());
	obj.AddMember("c", color, m_document.GetAllocator());
	m_document["t"].PushBack(std::move(obj), m_document.GetAllocator());
}

void ChatRichText::SetFadeSustainSecs(float fadeSustain) { m_document["fs"] = fadeSustain; }

void ChatRichText::SetFadeLengthSecs(float fadeLength) { m_document["fl"] = fadeLength; }

void ChatSendFromPlayer(unsigned int playerId, const char* text, bool isteam)
{
	unsigned int playerIndex = playerId + 1;

	g_SkipSayTextHook = true;
	CServerGameDLL__OnReceivedSayTextMessage(gServer, playerIndex, text, isteam ? 1 : 0);
}

void ChatWhisperFromPlayer(unsigned int fromPlayerId, unsigned int toPlayerId, const char* text)
{
	unsigned int fromPlayerIndex = fromPlayerId + 1;
	unsigned int toPlayerIndex = toPlayerId + 1;

	CBasePlayer* toPlayer = UTIL_PlayerByIndex(toPlayerIndex);
	if (toPlayer == NULL)
	{
		return;
	}

	CRecipientFilter filter;
	CRecipientFilter__Construct(&filter);
	CRecipientFilter__AddRecipient(&filter, toPlayer);
	CRecipientFilter__MakeReliable(&filter);

	UserMessageBegin(&filter, "SayText");
	MessageWriteByte(fromPlayerIndex);
	MessageWriteString(text);
	MessageWriteBool(false); // isteam=false
	MessageWriteBool(false); // isdead=false
	MessageEnd();

	CRecipientFilter__Destruct(&filter);
}

void ChatBroadcastText(const char* text)
{
	ChatRichText rich;
	rich.InsertColor(0xff, 0xff, 0xff, 0xff);
	rich.InsertText(text);
	ChatBroadcastRichText(rich);
}

void ChatWhisperText(unsigned int toPlayerId, const char* text)
{
	ChatRichText rich;
	rich.InsertColor(0xff, 0xff, 0xff, 0xff);
	rich.InsertText(text);
	ChatWhisperRichText(toPlayerId, rich);
}

constexpr char CHAT_PACKET_CONTINUE = 1;
constexpr char CHAT_PACKET_ANNOUNCE = 2;

void ChatBroadcastRichText(const ChatRichText& text)
{
	rapidjson::StringBuffer buffer;

	text.ToString(buffer);

	size_t remainingLength = buffer.GetSize();
	const char* textOffset = buffer.GetString();

	CRecipientFilter filter;
	CRecipientFilter__Construct(&filter);
	CRecipientFilter__AddAllPlayers(&filter);
	CRecipientFilter__MakeReliable(&filter);

	// Custom messages are formatted as JSON, so they can be pretty long.
	// SayText string payloads are limited to 255 characters long, so as a workaround
	// we send multiple separate chunks and construct them into one string on the other
	// side.
	// The first char is the packet ID. As a byte:
	//  1=not done
	//  2=announce
	while (true)
	{
		bool isLast = remainingLength <= 254;

		char sendText[256];
		sendText[0] = isLast ? CHAT_PACKET_ANNOUNCE : CHAT_PACKET_CONTINUE;
		strncpy(sendText + 1, textOffset, 254);
		sendText[255] = 0;

		UserMessageBegin(&filter, "SayText");
		MessageWriteByte(0); // playerIndex=0 for rich text
		MessageWriteString(sendText);
		MessageWriteBool(false); // isteam=false
		MessageWriteBool(false); // isdead=false
		MessageEnd();

		if (isLast)
		{
			break;
		}

		remainingLength -= 254;
		textOffset += 254;
	}

	CRecipientFilter__Destruct(&filter);
}

void ChatWhisperRichText(unsigned int toPlayerId, const ChatRichText& text)
{
	unsigned int toPlayerIndex = toPlayerId + 1;
	rapidjson::StringBuffer buffer;

	text.ToString(buffer);

	CBasePlayer* toPlayer = UTIL_PlayerByIndex(toPlayerIndex);
	if (toPlayer == NULL)
	{
		return;
	}

	CRecipientFilter filter;
	CRecipientFilter__Construct(&filter);
	CRecipientFilter__AddRecipient(&filter, toPlayer);
	CRecipientFilter__MakeReliable(&filter);

	UserMessageBegin(&filter, "SayText");
	MessageWriteByte(0); // playerIndex=0 for rich text
	MessageWriteString(buffer.GetString());
	MessageWriteBool(false); // isteam=false
	MessageWriteBool(false); // isdead=false
	MessageEnd();

	CRecipientFilter__Destruct(&filter);
}

// void function NSChatSendFromPlayer( int playerIndex, string text, bool isteam )
SQRESULT SQ_ChatSendFromPlayer(void* sqvm)
{
	int playerIndex = ServerSq_getinteger(sqvm, 1);
	const char* text = ServerSq_getstring(sqvm, 2);
	bool isteam = ServerSq_getbool(sqvm, 3);

	ChatSendFromPlayer(playerIndex, text, isteam);

	return SQRESULT_NULL;
}

// void function NSChatWhisperFromPlayer( int fromPlayerIndex, int toPlayerIndex, string text )
SQRESULT SQ_ChatWhisperFromPlayer(void* sqvm)
{
	int fromPlayerIndex = ServerSq_getinteger(sqvm, 1);
	int toPlayerIndex = ServerSq_getinteger(sqvm, 2);
	const char* text = ServerSq_getstring(sqvm, 3);

	ChatWhisperFromPlayer(fromPlayerIndex, toPlayerIndex, text);

	return SQRESULT_NULL;
}

// void function NSChatBroadcastText( string text )
SQRESULT SQ_ChatBroadcastText(void* sqvm)
{
	const char* text = ServerSq_getstring(sqvm, 1);

	ChatBroadcastText(text);

	return SQRESULT_NULL;
}

// void function NSChatBroadcastRainbowText( string text )
SQRESULT SQ_ChatBroadcastRainbowText(void* sqvm)
{
	const char* text = ServerSq_getstring(sqvm, 1);

	unsigned char colors[18];
	colors[0] = 0xFF;
	colors[1] = 0x7F;
	colors[2] = 0x7F;
	colors[3] = 0xFF;
	colors[4] = 0xFF;
	colors[5] = 0x7F;
	colors[6] = 0x7F;
	colors[7] = 0xFF;
	colors[8] = 0x7F;
	colors[9] = 0x7F;
	colors[10] = 0xFF;
	colors[11] = 0xFF;
	colors[12] = 0x7F;
	colors[13] = 0x7F;
	colors[14] = 0xFF;
	colors[15] = 0xFF;
	colors[16] = 0x7F;
	colors[17] = 0xFF;

	ChatRichText richText;
	size_t textLen = strlen(text);
	for (size_t charIndex = 0; charIndex < textLen; charIndex++)
	{
		size_t colorIndex = charIndex % 6;
		richText.InsertColor(colors[colorIndex * 3], colors[colorIndex * 3 + 1], colors[colorIndex * 3 + 2], 255);

		char mini[2];
		mini[0] = text[charIndex];
		mini[1] = 0;
		richText.InsertText(mini);
	}
	ChatBroadcastRichText(richText);

	return SQRESULT_NULL;
}

// void function NSChatWhisperText( int toPlayerIndex, string text )
SQRESULT SQ_ChatWhisperText(void* sqvm)
{
	int toPlayerIndex = ServerSq_getinteger(sqvm, 1);
	const char* text = ServerSq_getstring(sqvm, 2);

	ChatWhisperText(toPlayerIndex, text);

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
		hook, (char*)baseAddress + 0x1595C0, &CServerGameDLL__OnReceivedSayTextMessageHook,
		reinterpret_cast<LPVOID*>(&CServerGameDLL__OnReceivedSayTextMessage));

	// Chat intercept functions
	g_ServerSquirrelManager->AddFuncRegistration(
		"void", "NSSetMessage", "string message, int playerId, int channelId, bool shouldBlock", "", setMessage);
	g_ServerSquirrelManager->AddFuncRegistration("string", "NSChatGetCurrentMessage", "", "", getMessageServer);
	g_ServerSquirrelManager->AddFuncRegistration("int", "NSChatGetCurrentPlayer", "", "", getPlayerServer);
	g_ServerSquirrelManager->AddFuncRegistration("int", "NSChatGetCurrentChannel", "", "", getChannelServer);
	g_ServerSquirrelManager->AddFuncRegistration("bool", "NSShouldProcessMessage", "", "", getShouldProcessMessage);
	g_ServerSquirrelManager->AddFuncRegistration(
		"void", "NSPushMessage", "string message, int playerId, int channelId, bool shouldBlock", "", pushMessage);

	// Chat sending functions
	g_ServerSquirrelManager->AddFuncRegistration(
		"void", "NSChatSendFromPlayer", "int playerIndex, string text, bool isteam", "", SQ_ChatSendFromPlayer);
	g_ServerSquirrelManager->AddFuncRegistration(
		"void", "NSChatWhisperFromPlayer", "int fromPlayerIndex, int toPlayerIndex, string text", "", SQ_ChatWhisperFromPlayer);
	g_ServerSquirrelManager->AddFuncRegistration("void", "NSChatBroadcastText", "string text", "", SQ_ChatBroadcastText);
	g_ServerSquirrelManager->AddFuncRegistration("void", "NSChatBroadcastRainbowText", "string text", "", SQ_ChatBroadcastRainbowText);
	g_ServerSquirrelManager->AddFuncRegistration("void", "NSChatWhisperText", "int toPlayerIndex, string text", "", SQ_ChatWhisperText);
}
