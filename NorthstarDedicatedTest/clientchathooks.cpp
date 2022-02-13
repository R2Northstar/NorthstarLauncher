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
	vgui_Color m_networkNameColor;

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

enum class SwatchColor
{
	MainText,
	SameTeamName,
	EnemyTeamName,
	NetworkName,
};

SwatchColor swatchColors[4] = {
	SwatchColor::MainText,
	SwatchColor::SameTeamName,
	SwatchColor::EnemyTeamName,
	SwatchColor::NetworkName,
};

vgui_Color darkColors[8] = {vgui_Color{0, 0, 0, 255},	   vgui_Color{205, 49, 49, 255},  vgui_Color{13, 188, 121, 255},
							vgui_Color{229, 229, 16, 255}, vgui_Color{36, 114, 200, 255}, vgui_Color{188, 63, 188, 255},
							vgui_Color{17, 168, 205, 255}, vgui_Color{229, 229, 229, 255}};

vgui_Color lightColors[8] = {vgui_Color{102, 102, 102, 255}, vgui_Color{241, 76, 76, 255},	vgui_Color{35, 209, 139, 255},
							 vgui_Color{245, 245, 67, 255},	 vgui_Color{59, 142, 234, 255}, vgui_Color{214, 112, 214, 255},
							 vgui_Color{41, 184, 219, 255},	 vgui_Color{255, 255, 255, 255}};

static void LocalChatInsertChar(LocalChatContext context, wchar_t ch)
{
	for (CHudChat* hud = *gHudChatList; hud != NULL; hud = hud->next)
	{
		if (hud->m_unknownContext != (int)context)
			continue;

		hud->m_richText->vtable->InsertChar(hud->m_richText, ch);
	}
}

static void LocalChatInsertText(LocalChatContext context, const char* str)
{
	WCHAR messageUnicode[288];
	ConvertANSIToUnicode(str, -1, messageUnicode, 274);

	for (CHudChat* hud = *gHudChatList; hud != NULL; hud = hud->next)
	{
		if (hud->m_unknownContext != (int)context)
			continue;

		hud->m_richText->vtable->InsertStringWide(hud->m_richText, messageUnicode);
		hud->m_richText->vtable->InsertFade(hud->m_richText, 12.f, 1.f);
	}
}

static void LocalChatInsertColorChange(LocalChatContext context, vgui_Color color)
{
	for (CHudChat* hud = *gHudChatList; hud != NULL; hud = hud->next)
	{
		if (hud->m_unknownContext != (int)context)
			continue;

		hud->m_richText->vtable->InsertColorChange(hud->m_richText, color);
	}
}

static vgui_Color GetHudSwatchColor(CHudChat* hud, SwatchColor swatchColor)
{
	switch (swatchColor)
	{
	case SwatchColor::MainText:
		return hud->m_mainTextColor;
	case SwatchColor::SameTeamName:
		return hud->m_sameTeamColor;
	case SwatchColor::EnemyTeamName:
		return hud->m_enemyTeamColor;
	case SwatchColor::NetworkName:
		return hud->m_networkNameColor;
	}
	return vgui_Color{0, 0, 0, 0};
}

static void LocalChatInsertSwatchColorChange(LocalChatContext context, SwatchColor swatchColor)
{
	for (CHudChat* hud = *gHudChatList; hud != NULL; hud = hud->next)
	{
		if (hud->m_unknownContext != (int)context)
			continue;
		hud->m_richText->vtable->InsertColorChange(hud->m_richText, GetHudSwatchColor(hud, swatchColor));
	}
}

class AnsiEscapeDecoder
{
  public:
	explicit AnsiEscapeDecoder(LocalChatContext context) : m_context(context) {}

	void HandleVal(unsigned long val)
	{
		switch (m_next)
		{
		case Next::ControlType:
			m_next = HandleControlType(val);
			break;
		case Next::ForegroundType:
			m_next = HandleForegroundType(val);
			break;
		case Next::Foreground8Bit:
			m_next = HandleForeground8Bit(val);
			break;
		case Next::ForegroundR:
			m_next = HandleForegroundR(val);
			break;
		case Next::ForegroundG:
			m_next = HandleForegroundG(val);
			break;
		case Next::ForegroundB:
			m_next = HandleForegroundB(val);
			break;
		}
	}

  private:
	enum class Next
	{
		ControlType,
		ForegroundType,
		Foreground8Bit,
		ForegroundR,
		ForegroundG,
		ForegroundB
	};

	LocalChatContext m_context;
	Next m_next = Next::ControlType;
	vgui_Color m_expandedColor{0, 0, 0, 0};

	Next HandleControlType(unsigned long val)
	{
		// Reset
		if (val == 0 || val == 39)
		{
			LocalChatInsertSwatchColorChange(m_context, SwatchColor::MainText);
			return Next::ControlType;
		}

		// Dark foreground color
		if (val >= 30 && val < 38)
		{
			LocalChatInsertColorChange(m_context, darkColors[val - 30]);
			return Next::ControlType;
		}

		// Light foreground color
		if (val >= 90 && val < 98)
		{
			LocalChatInsertColorChange(m_context, lightColors[val - 90]);
			return Next::ControlType;
		}

		// Game swatch color
		if (val >= 110 && val < 114)
		{
			LocalChatInsertSwatchColorChange(m_context, swatchColors[val - 110]);
			return Next::ControlType;
		}

		// Expanded foreground color
		if (val == 38)
		{
			return Next::ForegroundType;
		}

		return Next::ControlType;
	}

	Next HandleForegroundType(unsigned long val)
	{
		// Next values are r,g,b
		if (val == 2)
		{
			m_expandedColor = {0, 0, 0, 255};
			return Next::ForegroundR;
		}
		// Next value is 8-bit swatch color
		if (val == 5)
		{
			return Next::Foreground8Bit;
		}

		// Invalid
		return Next::ControlType;
	}

	Next HandleForeground8Bit(unsigned long val)
	{
		if (val < 8)
		{
			LocalChatInsertColorChange(m_context, darkColors[val]);
		}
		else if (val < 16)
		{
			LocalChatInsertColorChange(m_context, lightColors[val - 8]);
		}
		else if (val < 232)
		{
			unsigned char code = val - 16;
			unsigned char blue = code % 6;
			unsigned char green = ((code - blue) / 6) % 6;
			unsigned char red = (code - blue - (green * 6)) / 36;
			LocalChatInsertColorChange(
				m_context, vgui_Color{(unsigned char)(red * 51), (unsigned char)(green * 51), (unsigned char)(blue * 51), 255});
		}
		else if (val < UCHAR_MAX)
		{
			unsigned char brightness = (val - 232) * 10 + 8;
			LocalChatInsertColorChange(m_context, vgui_Color{brightness, brightness, brightness, 255});
		}

		return Next::ControlType;
	}

	Next HandleForegroundR(unsigned long val)
	{
		if (val >= UCHAR_MAX) return Next::ControlType;

		m_expandedColor.r = (unsigned char)val;
		return Next::ForegroundG;
	}

	Next HandleForegroundG(unsigned long val)
	{
		if (val >= UCHAR_MAX) return Next::ControlType;

		m_expandedColor.g = (unsigned char)val;
		return Next::ForegroundB;
	}

	Next HandleForegroundB(unsigned long val)
	{
		if (val >= UCHAR_MAX) return Next::ControlType;

		m_expandedColor.b = (unsigned char)val;
		LocalChatInsertColorChange(m_context, m_expandedColor);
		return Next::ControlType;
	}
};

const char* ApplyAnsiEscape(LocalChatContext context, const char* escape)
{
	AnsiEscapeDecoder decoder(context);
	while (true)
	{
		char* afterControlType = NULL;
		unsigned long controlType = strtoul(escape, &afterControlType, 10);

		// Malformed cases:
		// afterControlType = NULL: strtoul errored
		// controlType = 0 and escape doesn't actually start with 0: wasn't a number
		if (afterControlType == NULL || (controlType == 0 && escape[0] != '0'))
		{
			return escape;
		}

		decoder.HandleVal(controlType);

		// m indicates the end of the sequence
		if (afterControlType[0] == 'm')
		{
			return afterControlType + 1;
		}

		// : or ; indicates more values remain, anything else is malformed
		if (afterControlType[0] != ':' && afterControlType[0] != ';')
		{
			return afterControlType;
		}

		escape = afterControlType + 1;
	}
}

void LocalChatWriteLine(LocalChatContext context, const char* str)
{
	char writeBuffer[256];

	// Force a new line and color reset at the start of all chat lines
	LocalChatInsertChar(context, L'\n');
	LocalChatInsertSwatchColorChange(context, SwatchColor::MainText);

	while (true)
	{
		const char* startOfEscape = strstr(str, "\033[");

		if (startOfEscape == NULL)
		{
			// No more escape sequences, write the remaining text and exit
			LocalChatInsertText(context, str);
			break;
		}

		if (startOfEscape != str)
		{
			// There is some text before the escape sequence, just print that

			size_t copyChars = startOfEscape - str;
			if (copyChars > 255)
				copyChars = 255;
			strncpy(writeBuffer, str, copyChars);
			writeBuffer[copyChars] = 0;

			LocalChatInsertText(context, writeBuffer);
		}

		const char* escape = startOfEscape + 2;
		str = ApplyAnsiEscape(context, escape);
	}
}

static void OnNetworkChatHook(const char* message, const char* playerName, int unknown)
{
	// todo: support squirrel hook
	OnNetworkChat(message, playerName, unknown);
}

enum class AnonymousMessageType : char
{
	Announce = 1,
};

static void CHudChat__AddGameLineHook(CHudChat* self, const char* message, int fromPlayerIndex, bool isteam, bool isdead)
{
	// Anonymous messages are from playerIndex=0, other messages are handled as normal
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

	char messageType = message[0];
	const char* messageContent = message + 1;

	if (messageType == (char)AnonymousMessageType::Announce)
	{
		LocalChatWriteLine(LocalChatContext::Game, messageContent);
	}
}

// void NSGameChatWriteLine( string text )
SQRESULT SQ_GameChatWriteLine(void* sqvm)
{
	const char* text = ClientSq_getstring(sqvm, 1);
	LocalChatWriteLine(LocalChatContext::Game, text);
	return SQRESULT_NULL;
}

// void NSNetworkChatWriteLine( string text )
SQRESULT SQ_NetworkChatWriteLine(void* sqvm)
{
	const char* text = ClientSq_getstring(sqvm, 1);
	LocalChatWriteLine(LocalChatContext::Network, text);
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
	g_ClientSquirrelManager->AddFuncRegistration("void", "NSNetworkChatWriteLine", "string text", "", SQ_NetworkChatWriteLine);
}
