#pragma once
#include "pch.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>

class ChatRichText
{
  public:
	ChatRichText();

	void ToString(rapidjson::StringBuffer& buffer) const;

	void InsertLocalized(const char* str);
	void InsertText(const char* str);
	void InsertColor(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha);

	void SetFadeSustainSecs(float fadeSustain);
	void SetFadeLengthSecs(float fadeLength);

  private:
	rapidjson::Document m_document;
};

void ChatSendFromPlayer(unsigned int playerId, const char* text, bool isteam);

void ChatWhisperFromPlayer(unsigned int fromPlayerId, unsigned int toPlayerId, const char* text);

void ChatBroadcastText(const char* text);

void ChatWhisperText(unsigned int toPlayerId, const char* text);

void ChatBroadcastRichText(const ChatRichText& text);

void ChatWhisperRichText(unsigned int toPlayerId, const ChatRichText& text);

void InitialiseServerChatHooks_Engine(HMODULE baseAddress);

void InitialiseServerChatHooks_Server(HMODULE baseAddress);
