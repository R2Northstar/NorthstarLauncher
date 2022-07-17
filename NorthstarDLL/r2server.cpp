#include "pch.h"
#include "r2server.h"

using namespace R2;

// use the R2 namespace for game funcs
namespace R2
{
	CBaseEntity* (*Server_GetEntityByIndex)(int index);
	CBasePlayer*(__fastcall* UTIL_PlayerByIndex)(int playerIndex);
} // namespace R2

ON_DLL_LOAD("server.dll", R2GameServer, (HMODULE baseAddress))
{
	Server_GetEntityByIndex = (CBaseEntity*(*)(int))((char*)baseAddress + 0xFB820);
	UTIL_PlayerByIndex = (CBasePlayer*(__fastcall*)(int))((char*)baseAddress + 0x26AA10);
}