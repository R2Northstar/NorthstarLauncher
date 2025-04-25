#include "r2client.h"
#include <core/vanilla.h>

char* g_pLocalPlayerUserID;
char* g_pLocalPlayerOriginToken;
GetBaseLocalClientType GetBaseLocalClient;

// fix rce expolit
typedef int(__fastcall* o_CBaseClientState_ProcessStringCmd_t)(int64_t a1, int64_t a2);
static o_CBaseClientState_ProcessStringCmd_t o_CBaseClientState_ProcessStringCmd;
static int h_CBaseClientState_ProcessStringCmd(int64_t a1, int64_t a2) {
	if (g_pVanillaCompatibility->GetVanillaCompatibility())
	{
		return o_CBaseClientState_ProcessStringCmd(a1, a2);
	}
	else
	{
		return 1;
	}
}


ON_DLL_LOAD("engine.dll", R2EngineClient, (CModule module))
{
	g_pLocalPlayerUserID = module.Offset(0x13F8E688).RCast<char*>();
	g_pLocalPlayerOriginToken = module.Offset(0x13979C80).RCast<char*>();
	o_CBaseClientState_ProcessStringCmd = module.Offset(0x1A1C20).RCast<o_CBaseClientState_ProcessStringCmd_t>();

	HookAttach(&(PVOID&)o_CBaseClientState_ProcessStringCmd, (PVOID)h_CBaseClientState_ProcessStringCmd);


	GetBaseLocalClient = module.Offset(0x78200).RCast<GetBaseLocalClientType>();
}
