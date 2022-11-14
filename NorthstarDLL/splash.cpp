#include "pch.h"
#include "splash.h"

typedef void (*SetSplashMessageExternal_t)(const char* msg, int progress, bool close);

SetSplashMessageExternal_t SetSplashMessageExternal;

void SetSplashMessage(const char* msg, int progress, bool close)
{
	SetSplashMessageExternal(msg, progress, close);
}

void InitialiseSplashScreen()
{
	SetSplashMessageExternal = (SetSplashMessageExternal_t)GetProcAddress(NULL, "SetSplashMessage");
}
