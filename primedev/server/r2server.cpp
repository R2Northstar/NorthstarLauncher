#include "r2server.h"

CBaseEntity* (*Server_GetEntityByIndex)(int index);
CBasePlayer*(__fastcall* UTIL_PlayerByIndex)(int playerIndex);

ON_DLL_LOAD("server.dll", R2GameServer, (CModule module))
{
	Server_GetEntityByIndex = module.Offset(0xFB820).RCast<CBaseEntity* (*)(int)>();
	UTIL_PlayerByIndex = module.Offset(0x26AA10).RCast<CBasePlayer*(__fastcall*)(int)>();

	// Remove call to Error for Geo bug: bullet trace ended at (%f, %f, %f), which is outside the max map coord:%i.
	module.Offset(0x43D4D8).NOP(6);
}
