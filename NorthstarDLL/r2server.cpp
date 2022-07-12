#include "pch.h"
#include "r2server.h"

using namespace R2;

// use the R2 namespace for game funcs
namespace R2
{
	server_state_t* g_pServerState;
	void* (*Server_GetEntityByIndex)(int index);
	CBasePlayer*(__fastcall* UTIL_PlayerByIndex)(int playerIndex);
} // namespace R2

ON_DLL_LOAD("engine.dll", R2EngineServer, (HMODULE baseAddress))
{
	g_pServerState = (server_state_t*)((char*)baseAddress + 0x12A53D48);
}

ON_DLL_LOAD("server.dll", R2GameServer, (HMODULE baseAddress))
{
	Server_GetEntityByIndex = (void*(*)(int))((char*)baseAddress + 0xFB820);
	UTIL_PlayerByIndex = (CBasePlayer*(__fastcall*)(int))((char*)baseAddress + 0x26AA10);
}