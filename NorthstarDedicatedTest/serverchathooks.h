#pragma once
#include "pch.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>

void ChatSendFromPlayer(unsigned int playerId, const char* text, bool isteam);

void ChatWhisperFromPlayer(unsigned int fromPlayerId, unsigned int toPlayerId, const char* text);

void ChatBroadcastText(const char* text);

void ChatWhisperText(unsigned int toPlayerId, const char* text);

void InitialiseServerChatHooks_Engine(HMODULE baseAddress);

void InitialiseServerChatHooks_Server(HMODULE baseAddress);
