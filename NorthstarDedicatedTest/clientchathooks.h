#pragma once
#include "pch.h"

constexpr float DEFAULT_FADE_SUSTAIN = 12.f;
constexpr float DEFAULT_FADE_LENGTH = 1.f;

enum class LocalChatContext
{
	Network = 0,
	Game = 1
};

void LocalChatStartLine(LocalChatContext context);

void LocalChatInsertLocalized(LocalChatContext context, const char* str);

void LocalChatInsertText(LocalChatContext context, const char* str);

void LocalChatInsertColor(LocalChatContext context, unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha);

void LocalChatInsertFade(LocalChatContext context, float sustainSecs, float lengthSecs);

void InitialiseClientChatHooks(HMODULE baseAddress);
