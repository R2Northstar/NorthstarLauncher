#include "pch.h"
#include "hookutils.h"
#include "concommand.h"

typedef void*(__fastcall* GetEntByIndexType)(int idx);
GetEntByIndexType GetEntByIndex;

static void* GetEntByIndexHook(int idx)
{
	if (idx >= 0x4000)
	{
		return nullptr;
	}
	return GetEntByIndex(idx);
}

void InitialiseGetEntByIndexServerFix(HMODULE baseAddress)
{
	HookEnabler hook;
	ENABLER_CREATEHOOK(hook, (char*)baseAddress + 0x2a8a50, &GetEntByIndexHook, reinterpret_cast<LPVOID*>(&GetEntByIndex));
}
