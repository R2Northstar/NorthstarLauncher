#include "pch.h"
#include "convar.h"
#include "gameutils.h"
#include "hoststate.h"

// cmd.h
Cbuf_GetCurrentPlayerType Cbuf_GetCurrentPlayer;
Cbuf_AddTextType Cbuf_AddText;
Cbuf_ExecuteType Cbuf_Execute;

// cengine stuff
CEngine* g_pEngine;
server_state_t* sv_m_State;

// server entity stuff
Server_GetEntityByIndexType Server_GetEntityByIndex;

// auth
char* g_LocalPlayerUserID;
char* g_LocalPlayerOriginToken;

// misc stuff
GetBaseLocalClientType GetBaseLocalClient;

void InitialiseEngineGameUtilFunctions(HMODULE baseAddress)
{
	Cbuf_GetCurrentPlayer = (Cbuf_GetCurrentPlayerType)((char*)baseAddress + 0x120630);
	Cbuf_AddText = (Cbuf_AddTextType)((char*)baseAddress + 0x1203B0);
	Cbuf_Execute = (Cbuf_ExecuteType)((char*)baseAddress + 0x1204B0);

	R2::g_pHostState = (R2::CHostState*)((char*)baseAddress + 0x7CF180);
	g_pEngine = *(CEngine**)((char*)baseAddress + 0x7D70C8);
	sv_m_State = (server_state_t*)((char*)baseAddress + 0x12A53D48);

	g_LocalPlayerUserID = (char*)baseAddress + 0x13F8E688;
	g_LocalPlayerOriginToken = (char*)baseAddress + 0x13979C80;

	GetBaseLocalClient = (GetBaseLocalClientType)((char*)baseAddress + 0x78200);

	/* NOTE:
		g_pCVar->FindVar("convar_name") now works. These are no longer needed.
		You can also itterate over every ConVar using CCVarIteratorInternal
		dump the pointers to a vector and access them from there.
	Example:
		std::vector<ConVar*> g_pAllConVars;
		for (auto& map : g_pCVar->DumpToMap())
		{
			ConVar* pConVar = g_pCVar->FindVar(map.first.c_str());
			if (pConVar)
			{
				g_pAllConVars.push_back(pConVar);
			}
		}*/
}

void InitialiseServerGameUtilFunctions(HMODULE baseAddress)
{
	Server_GetEntityByIndex = (Server_GetEntityByIndexType)((char*)baseAddress + 0xFB820);
}