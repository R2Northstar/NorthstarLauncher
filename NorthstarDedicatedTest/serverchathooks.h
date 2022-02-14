#pragma once
#include "pch.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>

void ChatSendFromPlayer(unsigned int playerId, const char* text, bool isteam);

void ChatBroadcastText(int playerIndex, const char* text, bool isTeam);

void InitialiseServerChatHooks_Engine(HMODULE baseAddress);

void InitialiseServerChatHooks_Server(HMODULE baseAddress);

#ifndef MESSAGETYPE
#define MESSAGETYPE
enum class AnonymousMessageType : char
{
	Announce = 1,
	Whisper = 2,
};
#endif