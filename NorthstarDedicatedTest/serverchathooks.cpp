#include "pch.h"
#include "serverchathooks.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

class CServerGameDLL;
class CBasePlayer;

class CRecipientFilter {
	char unknown[58];
};

CServerGameDLL* gServer;

typedef void(__fastcall* CServerGameDLL__OnReceivedSayTextMessageType)(CServerGameDLL* self, unsigned int senderPlayerIndex, const char* text, int channelId);
CServerGameDLL__OnReceivedSayTextMessageType CServerGameDLL__OnReceivedSayTextMessage;
CServerGameDLL__OnReceivedSayTextMessageType CServerGameDLL__OnReceivedSayTextMessageUnhooked;

typedef CBasePlayer* (__fastcall* UTIL_PlayerByIndexType)(int playerIndex);
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

bool gSkipSayTextHook = false;
static void CServerGameDLL__OnReceivedSayTextMessageHook(CServerGameDLL* self, unsigned int senderPlayerIndex, const char* text, int channelId) {
	if (gSkipSayTextHook) {
		gSkipSayTextHook = false;
		CServerGameDLL__OnReceivedSayTextMessageUnhooked(self, senderPlayerIndex, text, channelId);
		return;
	}

	// todo: impl other hook code here

	CServerGameDLL__OnReceivedSayTextMessageUnhooked(self, senderPlayerIndex, text, channelId);
}

ChatRichText::ChatRichText() {
	m_document.SetObject();
	m_document.AddMember("t", rapidjson::Value(rapidjson::kArrayType), m_document.GetAllocator());
}

void ChatRichText::ToString(rapidjson::StringBuffer& buffer) const {
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	m_document.Accept(writer);
}

void ChatRichText::InsertLocalized(const char* str) {
	rapidjson::Value obj(rapidjson::kObjectType);
	obj.AddMember("i", "l", m_document.GetAllocator());
	obj.AddMember("s", std::string(str), m_document.GetAllocator());
	m_document["t"].PushBack(std::move(obj), m_document.GetAllocator());
}

void ChatRichText::InsertText(const char* str) {
	rapidjson::Value obj(rapidjson::kObjectType);
	obj.AddMember("i", "t", m_document.GetAllocator());
	obj.AddMember("s", std::string(str), m_document.GetAllocator());
	m_document["t"].PushBack(std::move(obj), m_document.GetAllocator());
}

void ChatRichText::InsertColor(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha) {
	unsigned int color = (red << 24) + (green << 16) + (blue << 8) + alpha;

	rapidjson::Value obj(rapidjson::kObjectType);
	obj.AddMember("i", "c", m_document.GetAllocator());
	obj.AddMember("c", color, m_document.GetAllocator());
	m_document["t"].PushBack(std::move(obj), m_document.GetAllocator());
}

void ChatRichText::SetFadeSustainSecs(float fadeSustain) {
	m_document["fs"] = fadeSustain;
}

void ChatRichText::SetFadeLengthSecs(float fadeLength) {
	m_document["fl"] = fadeLength;
}

void ChatSendFromPlayer(unsigned int playerId, const char* text, bool isteam) {
	unsigned int playerIndex = playerId + 1;

	gSkipSayTextHook = true;
	CServerGameDLL__OnReceivedSayTextMessage(gServer, playerIndex, text, isteam ? 1 : 0);
}

void ChatWhisperFromPlayer(unsigned int fromPlayerId, unsigned int toPlayerId, const char* text) {
	unsigned int fromPlayerIndex = fromPlayerId + 1;
	unsigned int toPlayerIndex = toPlayerId + 1;

	CBasePlayer* toPlayer = UTIL_PlayerByIndex(toPlayerIndex);
	if (toPlayer == NULL) {
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

void ChatBroadcastText(const char* text) {
	ChatRichText rich;
	rich.InsertColor(0xff, 0xff, 0xff, 0xff);
	rich.InsertText(text);
	ChatBroadcastRichText(rich);
}

void ChatWhisperText(unsigned int toPlayerId, const char* text) {
	ChatRichText rich;
	rich.InsertColor(0xff, 0xff, 0xff, 0xff);
	rich.InsertText(text);
	ChatWhisperRichText(toPlayerId, rich);
}

constexpr char CHAT_PACKET_CONTINUE = 1;
constexpr char CHAT_PACKET_ANNOUNCE = 2;

void ChatBroadcastRichText(const ChatRichText& text) {
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
	while (true) {
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

		if (isLast) {
			break;
		}

		remainingLength -= 254;
		textOffset += 254;
	}

	CRecipientFilter__Destruct(&filter);
}

void ChatWhisperRichText(unsigned int toPlayerId, const ChatRichText& text) {
	unsigned int toPlayerIndex = toPlayerId + 1;
	rapidjson::StringBuffer buffer;

	text.ToString(buffer);

	CBasePlayer* toPlayer = UTIL_PlayerByIndex(toPlayerIndex);
	if (toPlayer == NULL) {
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

void InitialiseServerChatHooks_Engine(HMODULE baseAddress) {
	gServer = (CServerGameDLL*)((char*)baseAddress + 0x13F0AA98);
}

void InitialiseServerChatHooks_Server(HMODULE baseAddress) {
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
}
