#include "pch.h"
#include "r2engine.h"

using namespace R2;

// use the R2 namespace for game funcs
namespace R2
{
	Cbuf_GetCurrentPlayerType Cbuf_GetCurrentPlayer;
	Cbuf_AddTextType Cbuf_AddText;
	Cbuf_ExecuteType Cbuf_Execute;

	CEngine* g_pEngine;

	void (*CBaseClient__Disconnect)(void* self, uint32_t unknownButAlways1, const char* reason, ...);

	CBaseClient* g_pClientArray;

	server_state_t* g_pServerState;
} // namespace R2

ON_DLL_LOAD("engine.dll", R2Engine, (HMODULE baseAddress))
{
	Cbuf_GetCurrentPlayer = (Cbuf_GetCurrentPlayerType)((char*)baseAddress + 0x120630);
	Cbuf_AddText = (Cbuf_AddTextType)((char*)baseAddress + 0x1203B0);
	Cbuf_Execute = (Cbuf_ExecuteType)((char*)baseAddress + 0x1204B0);

	g_pEngine = *(CEngine**)((char*)baseAddress + 0x7D70C8);

	CBaseClient__Disconnect = (void (*)(void*, uint32_t, const char*, ...))((char*)baseAddress + 0x1012C0);

	g_pServerState = (server_state_t*)((char*)baseAddress + 0x12A53D48);

	g_pClientArray = (CBaseClient*)((char*)baseAddress + 0x12A53F90);
}