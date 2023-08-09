#include "dedicatedlogtoclient.h"

#include "core/convar/convar.h"
#include "engine/r2engine.h"
#include "logging/logging.h"

void (*CGameClient__ClientPrintf)(R2::CBaseClient* pClient, const char* fmt, ...);

void DediClientMsg(const char* pszMessage)
{
	if (R2::g_pServerState == NULL || R2::g_pCVar == NULL)
		return;

	if (*R2::g_pServerState == R2::server_state_t::ss_dead)
		return;

	enum class eSendPrintsToClient
	{
		NONE = -1,
		FIRST,
		ALL
	};

	static const ConVar* Cvar_dedi_sendPrintsToClient = R2::g_pCVar->FindVar("dedi_sendPrintsToClient");
	eSendPrintsToClient eSendPrints = static_cast<eSendPrintsToClient>(Cvar_dedi_sendPrintsToClient->GetInt());
	if (eSendPrints == eSendPrintsToClient::NONE)
		return;

	std::string sLogMessage = fmt::format("{}", pszMessage);
	for (int i = 0; i < R2::g_pGlobals->m_nMaxClients; i++)
	{
		R2::CBaseClient* pClient = &R2::g_pClientArray[i];

		if (pClient->m_Signon >= R2::eSignonState::CONNECTED)
		{
			CGameClient__ClientPrintf(pClient, sLogMessage.c_str());

			if (eSendPrints == eSendPrintsToClient::FIRST)
				break;
		}
	}
}

ON_DLL_LOAD_DEDI("engine.dll", DedicatedServerLogToClient, (CModule module))
{
	CGameClient__ClientPrintf = module.Offset(0x1016A0).RCast<void (*)(R2::CBaseClient*, const char*, ...)>();
}
