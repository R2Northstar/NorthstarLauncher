#pragma once
#include "pch.h"

enum class LocalChatContext
{
	Network = 0,
	Game = 1
};

void LocalChatWriteLine(LocalChatContext context, const char* str);

void InitialiseClientChatHooks(HMODULE baseAddress);
