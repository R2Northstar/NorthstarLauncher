#include "pch.h"
#include "gameutils.h"
#include "host_state.h"
#include "sv_rcon.h"
#include "cl_rcon.h"
#include "dedicated.h"

typedef void (*CHostState_FrameUpdateType)(CHostState* thisptr);
CHostState_FrameUpdateType CHostState__FrameUpdate;
CHostState* g_pHostState;

void CHostState__FrameUpdateHook(CHostState* thisptr)
{
	static bool bInitialized = false;

	if (!bInitialized)
	{
		if (IsDedicated())
		{
			Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec rcon_server", cmd_source_t::kCommandSrcCode);
			Cbuf_Execute();
			g_pRConServer->Init();
		}
		else
		{
			Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec rcon_client", cmd_source_t::kCommandSrcCode);
			Cbuf_Execute();
			g_pRConClient->Init();
		}
		bInitialized = true;
	}

	if (IsDedicated())
	{
		g_pRConServer->RunFrame();
	}
	else
	{
		g_pRConClient->RunFrame();
	}
	CHostState__FrameUpdate(thisptr);
}

void InitializeCHostStateHooks(HMODULE baseAddress)
{
	g_pHostState = (CHostState*)((char*)baseAddress + 0x7CF180);

	HookEnabler hook;
	ENABLER_CREATEHOOK(
		hook, (char*)baseAddress + 0x16DB00, &CHostState__FrameUpdateHook, reinterpret_cast<LPVOID*>(&CHostState__FrameUpdate));
}
