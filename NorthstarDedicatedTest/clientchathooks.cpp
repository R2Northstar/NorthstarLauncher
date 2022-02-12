#include "pch.h"
#include "clientchathooks.h"
#include <rapidjson/document.h>
#include "squirrel.h"

struct vgui_Color
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
	unsigned char a;
};

class vgui_BaseRichText_vtable;

class vgui_BaseRichText
{
  public:
	vgui_BaseRichText_vtable* vtable;
};

class vgui_BaseRichText_vtable
{
  public:
	char unknown1[1880];

	void(__fastcall* InsertChar)(vgui_BaseRichText* self, wchar_t ch);

	// yes these are swapped from the Source 2013 code, who knows why
	void(__fastcall* InsertStringWide)(vgui_BaseRichText* self, const wchar_t* wszText);
	void(__fastcall* InsertStringAnsi)(vgui_BaseRichText* self, const char* text);

	void(__fastcall* SelectNone)(vgui_BaseRichText* self);
	void(__fastcall* SelectAllText)(vgui_BaseRichText* self);
	void(__fastcall* SelectNoText)(vgui_BaseRichText* self);
	void(__fastcall* CutSelected)(vgui_BaseRichText* self);
	void(__fastcall* CopySelected)(vgui_BaseRichText* self);
	void(__fastcall* SetPanelInteractive)(vgui_BaseRichText* self, bool bInteractive);
	void(__fastcall* SetUnusedScrollbarInvisible)(vgui_BaseRichText* self, bool bInvis);

	void* unknown2;

	void(__fastcall* GotoTextStart)(vgui_BaseRichText* self);
	void(__fastcall* GotoTextEnd)(vgui_BaseRichText* self);

	void* unknown3[3];

	void(__fastcall* SetVerticalScrollbar)(vgui_BaseRichText* self, bool state);
	void(__fastcall* SetMaximumCharCount)(vgui_BaseRichText* self, int maxChars);
	void(__fastcall* InsertColorChange)(vgui_BaseRichText* self, vgui_Color col);
	void(__fastcall* InsertIndentChange)(vgui_BaseRichText* self, int pixelsIndent);
	void(__fastcall* InsertClickableTextStart)(vgui_BaseRichText* self, const char* pchClickAction);
	void(__fastcall* InsertClickableTextEnd)(vgui_BaseRichText* self);
	void(__fastcall* InsertPossibleURLString)(
		vgui_BaseRichText* self, const char* text, vgui_Color URLTextColor, vgui_Color normalTextColor);
	void(__fastcall* InsertFade)(vgui_BaseRichText* self, float flSustain, float flLength);
	void(__fastcall* ResetAllFades)(vgui_BaseRichText* self, bool bHold, bool bOnlyExpired, float flNewSustain);
	void(__fastcall* SetToFullHeight)(vgui_BaseRichText* self);
	int(__fastcall* GetNumLines)(vgui_BaseRichText* self);
};

class CHudChat
{
  public:
	char unknown1[720];

	vgui_Color m_sameTeamColor;
	vgui_Color m_enemyTeamColor;
	vgui_Color m_mainTextColor;
	vgui_Color m_lobbyNameColor;

	char unknown2[12];

	int m_unknownContext;

	char unknown3[8];

	vgui_BaseRichText* m_richText;

	CHudChat* next;
	CHudChat* previous;
};

// Linked list of CHudChats
CHudChat** gHudChatList;

typedef void(__fastcall* ConvertANSIToUnicodeType)(LPCSTR ansi, int ansiCharLength, LPWSTR unicode, int unicodeCharLength);
ConvertANSIToUnicodeType ConvertANSIToUnicode;

typedef void(__fastcall* OnNetworkChatType)(const char* message, const char* playerName, int unknown);
OnNetworkChatType OnNetworkChat;

typedef void(__fastcall* CHudChat__AddGameLineType)(CHudChat* self, const char* message, int fromPlayerIndex, bool isteam, bool isdead);
CHudChat__AddGameLineType CHudChat__AddGameLine;

void LocalChatStartLine(LocalChatContext context)
{
	for (CHudChat* hud = *gHudChatList; hud != NULL; hud = hud->next)
	{
		if (hud->m_unknownContext != (int)context)
			continue;

		hud->m_richText->vtable->InsertChar(hud->m_richText, L'\n');
	}
}

void LocalChatInsertLocalized(LocalChatContext context, const char* str)
{
	for (CHudChat* hud = *gHudChatList; hud != NULL; hud = hud->next)
	{
		if (hud->m_unknownContext != (int)context)
			continue;

		hud->m_richText->vtable->InsertStringAnsi(hud->m_richText, str);
	}
}

void LocalChatInsertText(LocalChatContext context, const char* str)
{
	WCHAR messageUnicode[288];
	ConvertANSIToUnicode(str, -1, messageUnicode, 274);

	for (CHudChat* hud = *gHudChatList; hud != NULL; hud = hud->next)
	{
		if (hud->m_unknownContext != (int)context)
			continue;

		hud->m_richText->vtable->InsertStringWide(hud->m_richText, messageUnicode);
	}
}

void LocalChatInsertColor(LocalChatContext context, unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha)
{
	for (CHudChat* hud = *gHudChatList; hud != NULL; hud = hud->next)
	{
		if (hud->m_unknownContext != (int)context)
			continue;

		hud->m_richText->vtable->InsertColorChange(hud->m_richText, vgui_Color{red, green, blue, alpha});
	}
}

void LocalChatInsertFade(LocalChatContext context, float sustainSecs, float lengthSecs)
{
	for (CHudChat* hud = *gHudChatList; hud != NULL; hud = hud->next)
	{
		if (hud->m_unknownContext != (int)context)
			continue;

		hud->m_richText->vtable->InsertFade(hud->m_richText, sustainSecs, lengthSecs);
	}
}

static void OnNetworkChatHook(const char* message, const char* playerName, int unknown)
{
	// todo: support squirrel hook
	OnNetworkChat(message, playerName, unknown);
}

static void OnChatAnnounce(const char* data)
{
	rapidjson::Document document;
	document.Parse(data);

	float fadeSustainSecs = document.HasMember("fs") ? document["fs"].GetFloat() : DEFAULT_FADE_SUSTAIN;
	float fadeLengthSecs = document.HasMember("fl") ? document["fl"].GetFloat() : DEFAULT_FADE_LENGTH;

	LocalChatStartLine(LocalChatContext::Game);

	rapidjson::Value::Array textItems = document["t"].GetArray();
	for (const rapidjson::Value& textItem : textItems)
	{
		const char* itemType = textItem["i"].GetString();

		if (!strcmp(itemType, "l"))
		{
			LocalChatInsertLocalized(LocalChatContext::Game, textItem["s"].GetString());
			LocalChatInsertFade(LocalChatContext::Game, fadeSustainSecs, fadeLengthSecs);
		}
		else if (!strcmp(itemType, "t"))
		{
			LocalChatInsertText(LocalChatContext::Game, textItem["s"].GetString());
			LocalChatInsertFade(LocalChatContext::Game, fadeSustainSecs, fadeLengthSecs);
		}
		else if (!strcmp(itemType, "c"))
		{
			unsigned int color = textItem["c"].GetUint();

			LocalChatInsertColor(LocalChatContext::Game, (color >> 24) & 0xFF, (color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
		}
	}
}

constexpr char CHAT_PACKET_CONTINUE = 1;
constexpr char CHAT_PACKET_ANNOUNCE = 2;

std::string g_IncomingBuffer;
bool g_IsIgnoringPacket = false;

static void CHudChat__AddGameLineHook(CHudChat* self, const char* message, int fromPlayerIndex, bool isteam, bool isdead)
{
	// Custom messages are from player=0, other messages are handled as normal
	if (fromPlayerIndex != 0)
	{
		// todo: support squirrel hook
		CHudChat__AddGameLine(self, message, fromPlayerIndex, isteam, isdead);
		return;
	}

	// This hook is called for every CHudChat instance, but we want to parse the announcement message and _then_ dispatch it to
	// each HUD. So ignore this call if this isn't the first HUD.
	if (self != *gHudChatList)
	{
		return;
	}

	if (message == NULL || message[0] == 0)
	{
		return;
	}

	char packetType = message[0];
	if (packetType != CHAT_PACKET_CONTINUE && g_IsIgnoringPacket)
	{
		// The packet that we were ignoring has ended, stop ignoring.
		g_IsIgnoringPacket = false;
		return;
	}
	else if (g_IsIgnoringPacket)
	{
		return;
	}

	g_IncomingBuffer.append(message + 1);

	// Limit the buffer at 64KiB, ignore the rest of it if we've reached that limit
	if (g_IncomingBuffer.size() > 65536)
	{
		g_IncomingBuffer.clear();
		g_IsIgnoringPacket = true;
		return;
	}

	if (packetType == CHAT_PACKET_CONTINUE)
	{
		return;
	}

	// Clear incoming data and keep the current packet string
	std::string packetData = std::move(g_IncomingBuffer);

	if (packetType == CHAT_PACKET_ANNOUNCE)
	{
		OnChatAnnounce(packetData.c_str());
	}
}

// void NSGameChatWriteLine( string text )
SQRESULT SQ_GameChatWriteLine(void* sqvm)
{
	const char* text = ClientSq_getstring(sqvm, 1);

	LocalChatStartLine(LocalChatContext::Game);
	LocalChatInsertColor(LocalChatContext::Game, 255, 255, 255, 255);
	LocalChatInsertText(LocalChatContext::Game, text);
	LocalChatInsertFade(LocalChatContext::Game, DEFAULT_FADE_SUSTAIN, DEFAULT_FADE_LENGTH);

	return SQRESULT_NULL;
}

// void NSGameChatStartLine()
SQRESULT SQ_GameChatStartLine(void* sqvm)
{
	LocalChatStartLine(LocalChatContext::Game);

	return SQRESULT_NULL;
}

// void NSGameChatInsertLocalized( string text )
SQRESULT SQ_GameChatInsertLocalized(void* sqvm)
{
	const char* text = ClientSq_getstring(sqvm, 1);

	LocalChatInsertLocalized(LocalChatContext::Game, text);
	LocalChatInsertFade(LocalChatContext::Game, DEFAULT_FADE_SUSTAIN, DEFAULT_FADE_LENGTH);

	return SQRESULT_NULL;
}

// void NSGameChatInsertText( string text )
SQRESULT SQ_GameChatInsertText(void* sqvm)
{
	const char* text = ClientSq_getstring(sqvm, 1);

	LocalChatInsertText(LocalChatContext::Game, text);
	LocalChatInsertFade(LocalChatContext::Game, DEFAULT_FADE_SUSTAIN, DEFAULT_FADE_LENGTH);

	return SQRESULT_NULL;
}

// void NSGameChatInsertColor( float red, float green, float blue, float alpha )
SQRESULT SQ_GameChatInsertColor(void* sqvm)
{
	float red = ClientSq_getfloat(sqvm, 1);
	float green = ClientSq_getfloat(sqvm, 2);
	float blue = ClientSq_getfloat(sqvm, 3);
	float alpha = ClientSq_getfloat(sqvm, 4);

	LocalChatInsertColor(
		LocalChatContext::Game, (unsigned char)(red * 255.f), (unsigned char)(green * 255.f), (unsigned char)(blue * 255.f),
		(unsigned char)(alpha * 255.f));

	return SQRESULT_NULL;
}

// void NSNetworkChatWriteLine( string text )
SQRESULT SQ_NetworkChatWriteLine(void* sqvm)
{
	const char* text = ClientSq_getstring(sqvm, 1);

	LocalChatStartLine(LocalChatContext::Network);
	LocalChatInsertColor(LocalChatContext::Network, 255, 255, 255, 255);
	LocalChatInsertText(LocalChatContext::Network, text);

	return SQRESULT_NULL;
}

// void NSNetworkChatStartLine()
SQRESULT SQ_NetworkChatStartLine(void* sqvm)
{
	LocalChatStartLine(LocalChatContext::Network);

	return SQRESULT_NULL;
}

// void NSNetworkChatInsertLocalized( string text )
SQRESULT SQ_NetworkChatInsertLocalized(void* sqvm)
{
	const char* text = ClientSq_getstring(sqvm, 1);

	LocalChatInsertLocalized(LocalChatContext::Network, text);

	return SQRESULT_NULL;
}

// void NSNetworkChatInsertText( string text )
SQRESULT SQ_NetworkChatInsertText(void* sqvm)
{
	const char* text = ClientSq_getstring(sqvm, 1);

	LocalChatInsertText(LocalChatContext::Network, text);

	return SQRESULT_NULL;
}

// void NSNetworkChatInsertColor( float red, float green, float blue, float alpha )
SQRESULT SQ_NetworkChatInsertColor(void* sqvm)
{
	float red = ClientSq_getfloat(sqvm, 1);
	float green = ClientSq_getfloat(sqvm, 2);
	float blue = ClientSq_getfloat(sqvm, 3);
	float alpha = ClientSq_getfloat(sqvm, 4);

	LocalChatInsertColor(
		LocalChatContext::Network, (unsigned char)(red * 255.f), (unsigned char)(green * 255.f), (unsigned char)(blue * 255.f),
		(unsigned char)(alpha * 255.f));

	return SQRESULT_NULL;
}

void InitialiseClientChatHooks(HMODULE baseAddress)
{
	gHudChatList = (CHudChat**)((char*)baseAddress + 0x11bA9E8);

	ConvertANSIToUnicode = (ConvertANSIToUnicodeType)((char*)baseAddress + 0x7339A0);

	HookEnabler hook;
	ENABLER_CREATEHOOK(hook, (char*)baseAddress + 0x22F160, &OnNetworkChatHook, reinterpret_cast<LPVOID*>(&OnNetworkChat));
	ENABLER_CREATEHOOK(hook, (char*)baseAddress + 0x22E580, &CHudChat__AddGameLineHook, reinterpret_cast<LPVOID*>(&CHudChat__AddGameLine));

	g_ClientSquirrelManager->AddFuncRegistration("void", "NSGameChatWriteLine", "string text", "", SQ_GameChatWriteLine);
	g_ClientSquirrelManager->AddFuncRegistration("void", "NSGameChatStartLine", "", "", SQ_GameChatStartLine);
	g_ClientSquirrelManager->AddFuncRegistration("void", "NSGameChatInsertLocalized", "string text", "", SQ_GameChatInsertLocalized);
	g_ClientSquirrelManager->AddFuncRegistration("void", "NSGameChatInsertText", "string text", "", SQ_GameChatInsertText);
	g_ClientSquirrelManager->AddFuncRegistration(
		"void", "NSGameChatInsertColor", "float red, float green, float blue, float alpha", "", SQ_GameChatInsertColor);
	g_ClientSquirrelManager->AddFuncRegistration("void", "NSNetworkChatWriteLine", "string text", "", SQ_NetworkChatWriteLine);
	g_ClientSquirrelManager->AddFuncRegistration("void", "NSNetworkChatStartLine", "", "", SQ_NetworkChatStartLine);
	g_ClientSquirrelManager->AddFuncRegistration("void", "NSNetworkChatInsertLocalized", "string text", "", SQ_NetworkChatInsertLocalized);
	g_ClientSquirrelManager->AddFuncRegistration("void", "NSNetworkChatInsertText", "string text", "", SQ_NetworkChatInsertText);
	g_ClientSquirrelManager->AddFuncRegistration(
		"void", "NSNetworkChatInsertColor", "float red, float green, float blue, float alpha", "", SQ_NetworkChatInsertColor);
}
