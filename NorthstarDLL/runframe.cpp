#include "pch.h"
#include "r2engine.h"
#include "r2server.h"
#include "hoststate.h"
#include "serverpresence.h"

AUTOHOOK_INIT()

AUTOHOOK(CEngine__Frame, engine.dll + 0x1C8650,
void, , (R2::CEngine* self))
{
	CEngine__Frame(self);
}

ON_DLL_LOAD("engine.dll", RunFrame, (HMODULE baseAddress))
{
	AUTOHOOK_DISPATCH()
}