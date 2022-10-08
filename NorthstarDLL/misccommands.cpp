#include "pch.h"
#include "misccommands.h"
#include "concommand.h"
#include "playlist.h"
#include "r2engine.h"
#include "r2client.h"
#include "tier0.h"
#include "hoststate.h"
#include "masterserver.h"
#include "modmanager.h"
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
	if (g_pSquirrel<ScriptContext::CLIENT>->m_pSQVM)
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

// fixes up various cvar flags to have more sane values
void FixupCvarFlags()
{
	if (Tier0::CommandLine()->CheckParm("-allowdevcvars"))
	{
		// strip hidden and devonly cvar flags
		int iNumCvarsAltered = 0;
		for (auto& pair : R2::g_pCVar->DumpToMap())
		{
			// strip flags
			int flags = pair.second->GetFlags();
			if (flags & FCVAR_DEVELOPMENTONLY)
			{
				flags &= ~FCVAR_DEVELOPMENTONLY;
				iNumCvarsAltered++;
			}

			if (flags & FCVAR_HIDDEN)
			{
				flags &= ~FCVAR_HIDDEN;
				iNumCvarsAltered++;
			}

			pair.second->m_nFlags = flags;
		}

		spdlog::info("Removed {} hidden/devonly cvar flags", iNumCvarsAltered);
	}

	const std::vector<std::tuple<const char*, uint32_t>> CVAR_FIXUP_ADD_FLAGS = {
		// system commands (i.e. necessary for proper functionality)
		// servers need to be able to disconnect
		{"disconnect", FCVAR_SERVER_CAN_EXECUTE},

		// clients need to be able to run these on server, which will then print the results of them on client
		{"status", FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS},
		{"ping", FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS},

		// cheat commands
		{"give", FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS},
		{"give_server", FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS},
		{"givecurrentammo", FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS},
		{"takecurrentammo", FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS},

		{"switchclass", FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS},
		{"set", FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS},
		{"_setClassVarServer", FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS},

		{"ent_create", FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS},
		{"ent_throw", FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS},
		{"ent_setname", FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS},
		{"ent_teleport", FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS},
		{"ent_remove", FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS},
		{"ent_remove_all", FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS},
		{"ent_fire", FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS},

		{"particle_create", FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS},
		{"particle_recreate", FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS},
		{"particle_kill", FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS},

		{"test_setteam", FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS},
		{"melee_lunge_ent", FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS}};

	const std::vector<std::tuple<const char*, uint32_t>> CVAR_FIXUP_REMOVE_FLAGS = {
		// unsure how this command works, not even sure it's used on retail servers, deffo shouldn't be used on northstar
		{"migrateme", FCVAR_SERVER_CAN_EXECUTE},
	};

	for (auto& fixup : CVAR_FIXUP_ADD_FLAGS)
		R2::g_pCVar->FindCommandBase(std::get<0>(fixup))->m_nFlags |= std::get<1>(fixup);

	for (auto& fixup : CVAR_FIXUP_REMOVE_FLAGS)
		R2::g_pCVar->FindCommandBase(std::get<0>(fixup))->m_nFlags &= ~std::get<1>(fixup);
}
