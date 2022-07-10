#include "pch.h"

AUTOHOOK_INIT()

bool* bIsOriginOverlayEnabled;

AUTOHOOK(OpenExternalWebBrowser, engine.dll + 0x184E40, 
void,, (char* pUrl, char flags))
{
	bool bIsOriginOverlayEnabledOriginal = *bIsOriginOverlayEnabled;
		if (flags & 2 && !strncmp(pUrl, "http", 4)) // custom force external browser flag
		*bIsOriginOverlayEnabled = false; // if this bool is false, game will use an external browser rather than the origin overlay one

	OpenExternalWebBrowser(pUrl, flags);
	*bIsOriginOverlayEnabled = bIsOriginOverlayEnabledOriginal;
}

ON_DLL_LOAD_CLIENT("engine.dll", ScriptExternalBrowserHooks, (HMODULE baseAddress))
{
	AUTOHOOK_DISPATCH()

	bIsOriginOverlayEnabled = (bool*)baseAddress + 0x13978255;
}