#include "pch.h"
#include "clientchathooks.h"
#include <rapidjson/document.h>
#include "squirrel.h"
#include "serverchathooks.h"
#include "localchatwriter.h"

/*class CBasePlayer_vtable;

class CBasePlayer {
public:
	CBasePlayer_vtable* vtable;
};

class CBasePlayer_vtable {
public:
	char unknown[712];

	int(__fastcall* GetTeam)(CBasePlayer* self);
};

// First parameter to GetPlayerName
void* gEngine;

typedef bool(__fastcall* GetPlayerNameType)(void* self, int playerIndex, char* outPlayerName);
GetPlayerNameType GetPlayerName;

typedef CBasePlayer* (__fastcall* UTIL_PlayerByIndexType)(int playerIndex);
UTIL_PlayerByIndexType UTIL_PlayerByIndex;

typedef CBasePlayer* (__fastcall* GetLocalPlayerType)();
GetLocalPlayerType GetLocalPlayer;

typedef void(__fastcall* OnNetworkChatType)(const char* message, const char* playerName, int unknown);
OnNetworkChatType OnNetworkChat;*/

typedef void(__fastcall* CHudChat__AddGameLineType)(void* self, const char* message, int fromPlayerIndex, bool isteam, bool isdead);
CHudChat__AddGameLineType CHudChat__AddGameLine;

/*static std::string currentMessage;
static int currentPlayerId;
static bool currentIsTeam;
static bool currentIsDead;
static bool currentIsEnemy;

static bool useCustomCode;
static unsigned char r;
static unsigned char g;
static unsigned char b;*/

struct ChatTags {
	bool whisper;
	bool team;
	bool dead;
};

struct Chat {
	std::string message;
	int senderId;
	bool isTeam;
	bool isDead;
	int type;
};
std::unique_ptr<Chat> currentChat;

/*static void WriteChatPrefixes(LocalChatWriter& writer, int fromPlayerId, ChatTags tags) {
	char senderName[256];
	CBasePlayer* localPlayer = GetLocalPlayer();
	CBasePlayer* senderPlayer = NULL;

	if (localPlayer == NULL) {
		return;
	}

	// Get the sender's name and player object if this isn't anonymous, aborting if either fail
	if (fromPlayerId >= 0) {
		int fromPlayerIndex = fromPlayerId + 1;
		if (!GetPlayerName(gEngine, fromPlayerIndex, senderName)) return;

		senderPlayer = UTIL_PlayerByIndex(fromPlayerIndex);
		if (senderPlayer == NULL) return;
	}

	// Force a new line
	writer.InsertChar(L'\n');

	// Add a [SERVER] tag to anonymous messages
	if (fromPlayerId < 0) {
		writer.InsertColorChange(vgui_Color{ 241, 76, 76, 255 });
		writer.InsertText(L"[SERVER]"); // todo: localize
	}

	// Add a [WHISPER] tag to whispers
	if (tags.whisper) {
		writer.InsertColorChange(vgui_Color{ 214, 112, 214, 255 });
		writer.InsertText(L"[WHISPER]"); // todo: localize
	}

	if (fromPlayerId >= 0) {
		int localTeam = localPlayer->vtable->GetTeam(localPlayer);
		int senderTeam = senderPlayer->vtable->GetTeam(senderPlayer);
		writer.InsertSwatchColorChange(localTeam == senderTeam
			? LocalChatWriter::SameTeamNameColor
			: LocalChatWriter::EnemyTeamNameColor);

		if (tags.dead) {
			writer.InsertText(L"[DEAD]"); // todo: localize
		}
		if (tags.team) {
			writer.InsertText(L"[TEAM]");
		}

		writer.InsertText(senderName);
		writer.InsertText(L": ");
		writer.InsertSwatchColorChange(LocalChatWriter::MainTextColor);
	}
}

static void OnReceivedChat(const char* message, int senderIndex, ChatTags tags, bool enableAdvancedParsing) {
	bool isAnonymous = senderIndex == 0;
	int senderId = senderIndex - 1;

	if (!isAnonymous) {
		// todo: process with squirrel chat hook
	}

	LocalChatWriter writer(LocalChatWriter::GameContext);
	WriteChatPrefixes(writer, senderId, tags);

	if (enableAdvancedParsing) {
		writer.Write(message);
	}
	else {
		writer.InsertText(message);
	}
}*/

static void CHudChat__AddGameLineHook(void* self, const char* message, int inboxIndex, bool isTeam, bool isDead) {
	int senderIndex = inboxIndex & CUSTOM_MESSAGE_INDEX_BIT;
	bool isAnonymous = senderIndex == 0;
	bool isCustom = isAnonymous || (inboxIndex & CUSTOM_MESSAGE_INDEX_MASK);

	// Type is set to 0 for non-custom messages, custom messages have a type encoded as the first byte
	int type = 0;
	const char* payload = message;
	if (isCustom) {
		type = message[0];
		payload = message + 1;
	}

	currentChat.reset();
	currentChat->message = payload;
	currentChat->senderId = senderIndex - 1;
	currentChat->isTeam = isTeam;
	currentChat->isDead = isDead;
	currentChat->type = type;
	g_ClientSquirrelManager->ExecuteCode("CHudChat_ProcessMessageStartThread()");
}

// void NSChatWrite( int context, string str )
static SQRESULT SQ_ChatWrite(void* sqvm) {
	int context = ClientSq_getinteger(sqvm, 1);
	const char* str = ClientSq_getstring(sqvm, 2);

	LocalChatWriter((LocalChatWriter::Context)context).Write(str);
	return SQRESULT_NOTNULL;
}

// void NSChatWriteRaw( int context, string str )
static SQRESULT SQ_ChatWriteRaw(void* sqvm) {
	int context = ClientSq_getinteger(sqvm, 1);
	const char* str = ClientSq_getstring(sqvm, 2);

	LocalChatWriter((LocalChatWriter::Context)context).InsertText(str);
	return SQRESULT_NOTNULL;
}

// void NSChatWriteLine( int context, string str )
static SQRESULT SQ_ChatWriteLine(void* sqvm) {
	int context = ClientSq_getinteger(sqvm, 1);
	const char* str = ClientSq_getstring(sqvm, 2);

	LocalChatWriter((LocalChatWriter::Context)context).WriteLine(str);
	return SQRESULT_NOTNULL;
}

// string NSChatGetCurrentMessage()
static SQRESULT SQ_ChatGetCurrentMessage(void* sqvm) {
	if (currentChat == NULL) {
		ClientSq_pusherror(sqvm, "No chat is available");
		return SQRESULT_ERROR;
	}

	ClientSq_pushstring(sqvm, currentChat->message.c_str(), -1);
	return SQRESULT_NOTNULL;
}

// int NSChatGetCurrentPlayer()
static SQRESULT SQ_ChatGetCurrentPlayer(void* sqvm) {
	if (currentChat == NULL) {
		ClientSq_pusherror(sqvm, "No chat is available");
		return SQRESULT_ERROR;
	}

	ClientSq_pushinteger(sqvm, currentChat->senderId);
	return SQRESULT_NOTNULL;
}

// bool NSChatGetIsTeam()
static SQRESULT SQ_ChatGetIsTeam(void* sqvm) {
	if (currentChat == NULL) {
		ClientSq_pusherror(sqvm, "No chat is available");
		return SQRESULT_ERROR;
	}

	ClientSq_pushbool(sqvm, currentChat->isTeam);
	return SQRESULT_NOTNULL;
}

// bool NSChatGetIsDead()
static SQRESULT SQ_ChatGetIsDead(void* sqvm) {
	if (currentChat == NULL) {
		ClientSq_pusherror(sqvm, "No chat is available");
		return SQRESULT_ERROR;
	}

	ClientSq_pushbool(sqvm, currentChat->isDead);
	return SQRESULT_NOTNULL;
}

// int NSChatGetCurrentType()
static SQRESULT SQ_ChatGetCurrentType(void* sqvm) {
	if (currentChat == NULL) {
		ClientSq_pusherror(sqvm, "No chat is available");
		return SQRESULT_ERROR;
	}

	ClientSq_pushinteger(sqvm, currentChat->type);
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
