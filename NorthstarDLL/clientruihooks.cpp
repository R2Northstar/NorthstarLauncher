#include "pch.h"
#include "convar.h"

AUTOHOOK_INIT()

ConVar* Cvar_rui_drawEnable;

AUTOHOOK(DrawRUIFunc, engine.dll + 0xFC500,
bool,, (void* a1, float* a2))
{
	if (!Cvar_rui_drawEnable->GetBool())
		return 0;
	
	return DrawRUIFunc(a1, a2);
}

ON_DLL_LOAD_CLIENT_RELIESON("engine.dll", RUI, ConVar, (CModule module))
{
	AUTOHOOK_DISPATCH()

	Cvar_rui_drawEnable = new ConVar("rui_drawEnable", "1", FCVAR_CLIENTDLL, "Controls whether RUI should be drawn");
}