#include "pch.h"
#include "hooks.h"
#include "scriptbrowserhooks.h"
#include "hookutils.h"

bool* bIsOriginOverlayEnabled;

typedef void (*OpenExternalWebBrowserType)(char* url, char flags);
OpenExternalWebBrowserType OpenExternalWebBrowser;
void OpenExternalWebBrowserHook(char* url, char flags)
{
	bool bIsOriginOverlayEnabledOriginal = *bIsOriginOverlayEnabled;
	if (flags & 2 && !strncmp(url, "http", 4)) // custom force external browser flag
		*bIsOriginOverlayEnabled = false; // if this bool is false, game will use an external browser rather than the origin overlay one

	OpenExternalWebBrowser(url, flags);
	*bIsOriginOverlayEnabled = bIsOriginOverlayEnabledOriginal;
}

ON_DLL_LOAD_CLIENT("engine.dll", ScriptExternalBrowserHooks, [](HMODULE baseAddress)
{
	bIsOriginOverlayEnabled = (bool*)baseAddress + 0x13978255;

	HookEnabler hook;
	ENABLER_CREATEHOOK(
		hook, (char*)baseAddress + 0x184E40, &OpenExternalWebBrowserHook, reinterpret_cast<LPVOID*>(&OpenExternalWebBrowser));
})