#include "pch.h"
#include "misccommands.h"
#include "concommand.h"
#include "playlist.h"
#include "r2engine.h"
#include "r2client.h"
#include "hoststate.h"
#include "masterserver.h"
#include "serverauthentication.h"
#include "squirrel.h"

void ConCommand_force_newgame(const CCommand& arg)
{
	if (arg.ArgC() < 2)
		return;

	R2::g_pHostState->m_iNextState = R2::HostState_t::HS_NEW_GAME;
	strncpy(R2::g_pHostState->m_levelName, arg.Arg(1), sizeof(R2::g_pHostState->m_levelName));
}

void ConCommand_ns_start_reauth_and_leave_to_lobby(const CCommand& arg)
{
	// hack for special case where we're on a local server, so we erase our own newly created auth data on disconnect
	g_pMasterServerManager->m_bNewgameAfterSelfAuth = true;
	g_pMasterServerManager->AuthenticateWithOwnServer(R2::g_pLocalPlayerUserID, g_pMasterServerManager->m_sOwnClientAuthToken);
}

void ConCommand_ns_end_reauth_and_leave_to_lobby(const CCommand& arg)
{
	R2::Cbuf_AddText(
		R2::Cbuf_GetCurrentPlayer(),
		fmt::format("serverfilter {}", g_pServerAuthentication->m_RemoteAuthenticationData.begin()->first).c_str(),
		R2::cmd_source_t::kCommandSrcCode);
	R2::Cbuf_Execute();

	// weird way of checking, but check if client script vm is initialised, mainly just to allow players to cancel this
	if (g_pSquirrel<ScriptContext::CLIENT>->SquirrelVM)
	{
		g_pServerAuthentication->m_bNeedLocalAuthForNewgame = true;

		// this won't set playlist correctly on remote clients, don't think they can set playlist until they've left which sorta
		// fucks things should maybe set this in HostState_NewGame?
		R2::SetCurrentPlaylist("tdm");
		strcpy(R2::g_pHostState->m_levelName, "mp_lobby");
		R2::g_pHostState->m_iNextState = R2::HostState_t::HS_NEW_GAME;
	}
}

void AddMiscConCommands()
{
	RegisterConCommand(
		"force_newgame",
		ConCommand_force_newgame,
		"forces a map load through directly setting g_pHostState->m_iNextState to HS_NEW_GAME",
		FCVAR_NONE);

	RegisterConCommand(
		"ns_start_reauth_and_leave_to_lobby",
		ConCommand_ns_start_reauth_and_leave_to_lobby,
		"called by the server, used to reauth and return the player to lobby when leaving a game",
		FCVAR_SERVER_CAN_EXECUTE);

	// this is a concommand because we make a deferred call to it from another thread
	RegisterConCommand("ns_end_reauth_and_leave_to_lobby", ConCommand_ns_end_reauth_and_leave_to_lobby, "", FCVAR_NONE);
}
