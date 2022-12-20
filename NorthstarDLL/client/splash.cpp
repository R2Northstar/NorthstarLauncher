#include "pch.h"
#include "splash.h"

typedef void (*SetSplashMessageExternal_t)(const char* msg, int progress, bool close);

AUTOHOOK_INIT()

// clang-format off
AUTOHOOK(CDedicatedAppSystemGroupStarter, engine.dll + 0x1C78F0,
void, __fastcall, (void* a1))
// clang-format on
{
	SetSplashMessage("Starting game", 8);
	return CDedicatedAppSystemGroupStarter(a1);
}

static bool hasPassedBikLoader = false;
// clang-format off
AUTOHOOK(BikInterfaceLoader, engine.dll + 0x46320,
void, __fastcall, (__int64 a1, __int64 a2))
// clang-format on
{
	if (!hasPassedBikLoader) // Only set the splash progress first time
	{
		hasPassedBikLoader = true;
		SetSplashMessage("Starting game engine", 7);
	}
	return BikInterfaceLoader(a1, a2);
}

SetSplashMessageExternal_t SetSplashMessageExternal;

void SetSplashMessage(const char* msg, int progress, bool close)
{
	if (SetSplashMessageExternal)
		SetSplashMessageExternal(msg, progress, close);
}

void InitialiseSplashScreen()
{
	HMODULE splashDLL;
	GetModuleHandleExA(NULL, "splash.dll", &splashDLL);
	SetSplashMessageExternal = (SetSplashMessageExternal_t)GetProcAddress(splashDLL, "SetSplashMessage");
}

ON_DLL_LOAD("engine.dll", Splash, (CModule module))
{
	AUTOHOOK_DISPATCH()
}
