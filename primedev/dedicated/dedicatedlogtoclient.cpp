#include "dedicatedlogtoclient.h"

#include "core/convar/convar.h"
#include "engine/r2engine.h"
#include "logging/logging.h"

void (*CGameClient__ClientPrintf)(CBaseClient* pClient, const char* fmt, ...);

void DediClientMsg(const char* pszMessage)
{
	if (g_pServerState == NULL || g_pCVar == NULL)
		return;

	if (*g_pServerState == server_state_t::ss_dead)
		return;

	enum class eSendPrintsToClient
	{
		NONE = -1,
		FIRST,
		ALL
	};

	static const ConVar* Cvar_dedi_sendPrintsToClient = g_pCVar->FindVar("dedi_sendPrintsToClient");
	eSendPrintsToClient eSendPrints = static_cast<eSendPrintsToClient>(Cvar_dedi_sendPrintsToClient->GetInt());
	if (eSendPrints == eSendPrintsToClient::NONE)
		return;

	std::string sLogMessage = fmt::format("{}", pszMessage);
	for (int i = 0; i < g_pGlobals->m_nMaxClients; i++)
	{
		CBaseClient* pClient = &g_pClientArray[i];

		if (pClient->m_Signon >= eSignonState::CONNECTED)
		{
			CGameClient__ClientPrintf(pClient, sLogMessage.c_str());

			if (eSendPrints == eSendPrintsToClient::FIRST)
				break;
		}
	}
}

ON_DLL_LOAD_DEDI("engine.dll", DedicatedServerLogToClient, (CModule module))
{
	CGameClient__ClientPrintf = module.Offset(0x1016A0).RCast<void (*)(CBaseClient*, const char*, ...)>();
}
