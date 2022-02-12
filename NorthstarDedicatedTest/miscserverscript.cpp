#include "pch.h"
#include "miscserverscript.h"
#include "squirrel.h"
#include "masterserver.h"
#include "serverauthentication.h"
#include "gameutils.h"
#include "serverchathooks.h"

// annoying helper function because i can't figure out getting players or entities from sqvm rn
// wish i didn't have to do it like this, but here we are
void* GetPlayerByIndex(int playerIndex)
{
	const int PLAYER_ARRAY_OFFSET = 0x12A53F90;
	const int PLAYER_SIZE = 0x2D728;

	void* playerArrayBase = (char*)GetModuleHandleA("engine.dll") + PLAYER_ARRAY_OFFSET;
	void* player = (char*)playerArrayBase + (playerIndex * PLAYER_SIZE);

	return player;
}

// void function NSEarlyWritePlayerIndexPersistenceForLeave( int playerIndex )
SQRESULT SQ_EarlyWritePlayerIndexPersistenceForLeave(void* sqvm)
{
	int playerIndex = ServerSq_getinteger(sqvm, 1);
	void* player = GetPlayerByIndex(playerIndex);

	if (!g_ServerAuthenticationManager->m_additionalPlayerData.count(player))
	{
		ServerSq_pusherror(sqvm, fmt::format("Invalid playerindex {}", playerIndex).c_str());
		return SQRESULT_ERROR;
	}

	g_ServerAuthenticationManager->m_additionalPlayerData[player].needPersistenceWriteOnLeave = false;
	g_ServerAuthenticationManager->WritePersistentData(player);
	return SQRESULT_NULL;
}

// bool function NSIsWritingPlayerPersistence()
SQRESULT SQ_IsWritingPlayerPersistence(void* sqvm)
{
	ServerSq_pushbool(sqvm, g_MasterServerManager->m_savingPersistentData);
	return SQRESULT_NOTNULL;
}

// bool function NSIsPlayerIndexLocalPlayer( int playerIndex )
SQRESULT SQ_IsPlayerIndexLocalPlayer(void* sqvm)
{
	int playerIndex = ServerSq_getinteger(sqvm, 1);
	void* player = GetPlayerByIndex(playerIndex);
	if (!g_ServerAuthenticationManager->m_additionalPlayerData.count(player))
	{
		ServerSq_pusherror(sqvm, fmt::format("Invalid playerindex {}", playerIndex).c_str());
		return SQRESULT_ERROR;
	}

	ServerSq_pushbool(sqvm, !strcmp(g_LocalPlayerUserID, (char*)player + 0xF500));
	return SQRESULT_NOTNULL;
}

// void function NSChatSendFromPlayer( int playerIndex, string text, bool isteam )
SQRESULT SQ_ChatSendFromPlayer(void* sqvm) {
	int playerIndex = ServerSq_getinteger(sqvm, 1);
	const char* text = ServerSq_getstring(sqvm, 2);
	bool isteam = ServerSq_getbool(sqvm, 3);

	ChatSendFromPlayer(playerIndex, text, isteam);

	return SQRESULT_NULL;
}

// void function NSChatWhisperFromPlayer( int fromPlayerIndex, int toPlayerIndex, string text )
SQRESULT SQ_ChatWhisperFromPlayer(void* sqvm) {
	int fromPlayerIndex = ServerSq_getinteger(sqvm, 1);
	int toPlayerIndex = ServerSq_getinteger(sqvm, 2);
	const char* text = ServerSq_getstring(sqvm, 3);

	ChatWhisperFromPlayer(fromPlayerIndex, toPlayerIndex, text);

	return SQRESULT_NULL;
}

// void function NSChatBroadcastText( string text )
SQRESULT SQ_ChatBroadcastText(void* sqvm) {
	const char* text = ServerSq_getstring(sqvm, 1);

	ChatBroadcastText(text);

	return SQRESULT_NULL;
}

// void function NSChatBroadcastRainbowText( string text )
SQRESULT SQ_ChatBroadcastRainbowText(void* sqvm) {
	const char* text = ServerSq_getstring(sqvm, 1);

	unsigned char colors[18];
	colors[0] = 0xFF; colors[1] = 0x7F; colors[2] = 0x7F;
	colors[3] = 0xFF; colors[4] = 0xFF; colors[5] = 0x7F;
	colors[6] = 0x7F; colors[7] = 0xFF; colors[8] = 0x7F;
	colors[9] = 0x7F; colors[10] = 0xFF; colors[11] = 0xFF;
	colors[12] = 0x7F; colors[13] = 0x7F; colors[14] = 0xFF;
	colors[15] = 0xFF; colors[16] = 0x7F; colors[17] = 0xFF;

	ChatRichText richText;
	size_t textLen = strlen(text);
	for (size_t charIndex = 0; charIndex < textLen; charIndex++) {
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
SQRESULT SQ_ChatWhisperText(void* sqvm) {
	int toPlayerIndex = ServerSq_getinteger(sqvm, 1);
	const char* text = ServerSq_getstring(sqvm, 2);

	ChatWhisperText(toPlayerIndex, text);

	return SQRESULT_NULL;
}

void InitialiseMiscServerScriptCommand(HMODULE baseAddress)
{
	g_ServerSquirrelManager->AddFuncRegistration(
		"void", "NSEarlyWritePlayerIndexPersistenceForLeave", "int playerIndex", "", SQ_EarlyWritePlayerIndexPersistenceForLeave);
	g_ServerSquirrelManager->AddFuncRegistration("bool", "NSIsWritingPlayerPersistence", "", "", SQ_IsWritingPlayerPersistence);
	g_ServerSquirrelManager->AddFuncRegistration("bool", "NSIsPlayerIndexLocalPlayer", "int playerIndex", "", SQ_IsPlayerIndexLocalPlayer);
	g_ServerSquirrelManager->AddFuncRegistration("void", "NSChatSendFromPlayer", "int playerIndex, string text, bool isteam", "", SQ_ChatSendFromPlayer);
	g_ServerSquirrelManager->AddFuncRegistration("void", "NSChatWhisperFromPlayer", "int fromPlayerIndex, int toPlayerIndex, string text", "", SQ_ChatWhisperFromPlayer);
	g_ServerSquirrelManager->AddFuncRegistration("void", "NSChatBroadcastText", "string text", "", SQ_ChatBroadcastText);
	g_ServerSquirrelManager->AddFuncRegistration("void", "NSChatBroadcastRainbowText", "string text", "", SQ_ChatBroadcastRainbowText);
	g_ServerSquirrelManager->AddFuncRegistration("void", "NSChatWhisperText", "int toPlayerIndex, string text", "", SQ_ChatWhisperText);
}
