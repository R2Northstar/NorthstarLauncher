#include "client/r2client.h"
#include "core/convar/convar.h"
#include "core/vanilla.h"
#include "masterserver/masterserver.h"

AUTOHOOK_INIT()

ConVar* Cvar_ns_has_agreed_to_send_token;

// mirrored in script
const int NOT_DECIDED_TO_SEND_TOKEN = 0;
const int AGREED_TO_SEND_TOKEN = 1;
const int DISAGREED_TO_SEND_TOKEN = 2;

// clang-format off
AUTOHOOK(AuthWithStryder, engine.dll + 0x1843A0,
void, __fastcall, (void* a1))
// clang-format on
{
	if (!g_pMasterServerManager->m_bOriginAuthWithMasterServerDone && Cvar_ns_has_agreed_to_send_token->GetInt() != DISAGREED_TO_SEND_TOKEN)
	{
		// if player has agreed to send token and we aren't already authing, try to auth
		if (Cvar_ns_has_agreed_to_send_token->GetInt() == AGREED_TO_SEND_TOKEN &&
			!g_pMasterServerManager->m_bOriginAuthWithMasterServerInProgress)
			g_pMasterServerManager->AuthenticateOriginWithMasterServer(g_pLocalPlayerUserID, g_pLocalPlayerOriginToken);
	}

	AuthWithStryder(a1);
}

ON_DLL_LOAD_CLIENT_RELIESON("engine.dll", ClientAuthHooks, ConVar, (CModule module))
{
	AUTOHOOK_DISPATCH()

	// this cvar will save to cfg once initially agreed with
	Cvar_ns_has_agreed_to_send_token = new ConVar(
		"ns_has_agreed_to_send_token",
		"0",
		FCVAR_ARCHIVE_PLAYERPROFILE,
		"whether the user has agreed to send their origin token to the northstar masterserver");
}
