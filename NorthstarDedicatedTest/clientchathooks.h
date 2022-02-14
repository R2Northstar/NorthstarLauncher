#pragma once
#include "pch.h"
#include "serverchathooks.h"

enum class LocalChatContext
{
	Network = 0,
	Game = 1
};

void LocalChatWriteLine(LocalChatContext context, const char* str, int fromPlayerId, AnonymousMessageType messageType);

void InitialiseClientChatHooks(HMODULE baseAddress);
