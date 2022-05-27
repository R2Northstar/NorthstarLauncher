#include "pch.h"
#include "r2server.h"

using namespace R2;

// use the R2 namespace for game funcs
namespace R2
{
	server_state_t* g_pServerState;
	Server_GetEntityByIndexType Server_GetEntityByIndex;
} // namespace R2

ON_DLL_LOAD("engine.dll", R2EngineServer, [](HMODULE baseAddress)
{
	g_pServerState = (server_state_t*)((char*)baseAddress + 0x12A53D48);
})

ON_DLL_LOAD("server.dll", R2GameServer, [](HMODULE baseAddress)
{
	Server_GetEntityByIndex = (Server_GetEntityByIndexType)((char*)baseAddress + 0xFB820);
})