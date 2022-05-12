#include "pch.h"
#include "hoststate.h"
#include "masterserver.h"
#include "serverauthentication.h"
#include "playlist.h"
#include "tier0.h"
#include "r2engine.h"

using namespace R2;

// use the R2 namespace for game funcs
namespace R2
{
	CHostState* g_pHostState;
} // namespace R2

typedef void (*CHostState__State_NewGameType)(CHostState* hostState);
CHostState__State_NewGameType CHostState__State_NewGame;
void CHostState__State_NewGameHook(CHostState* hostState)
{
	spdlog::info("HostState: NewGame");

	Cbuf_AddText(Cbuf_GetCurrentPlayer(), "exec autoexec_ns_server", cmd_source_t::kCommandSrcCode);
	Cbuf_Execute();

	// need to do this to ensure we don't go to private match
	if (g_ServerAuthenticationManager->m_bNeedLocalAuthForNewgame)
		SetCurrentPlaylist("tdm");

	// net_data_block_enabled is required for sp, force it if we're on an sp map
	// sucks for security but just how it be
	if (!strncmp(g_pHostState->m_levelName, "sp_", 3))
		g_pCVar->FindVar("net_data_block_enabled")->SetValue(true);

	double dStartTime = Tier0::Plat_FloatTime();
	CHostState__State_NewGame(hostState);
	spdlog::info("loading took {}s", Tier0::Plat_FloatTime() - dStartTime);

	int maxPlayers = 6;
	const char* maxPlayersVar = GetCurrentPlaylistVar("max_players", false);
	if (maxPlayersVar) // GetCurrentPlaylistVar can return null so protect against this
		maxPlayers = std::stoi(maxPlayersVar);

	// Copy new server name cvar to source
	Cvar_hostname->SetValue(Cvar_ns_server_name->GetString());

	g_MasterServerManager->AddSelfToServerList(
		Cvar_hostport->GetInt(),
		Cvar_ns_player_auth_port->GetInt(),
		Cvar_ns_server_name->GetString(),
		Cvar_ns_server_desc->GetString(),
		hostState->m_levelName,
		GetCurrentPlaylistName(),
		maxPlayers,
		Cvar_ns_server_password->GetString());
	g_ServerAuthenticationManager->StartPlayerAuthServer();
	g_ServerAuthenticationManager->m_bNeedLocalAuthForNewgame = false;
}

typedef void (*CHostState__State_ChangeLevelMPType)(CHostState* hostState);
CHostState__State_ChangeLevelMPType CHostState__State_ChangeLevelMP;
void CHostState__State_ChangeLevelMPHook(CHostState* hostState)
{
	spdlog::info("HostState: ChangeLevelMP");

	int maxPlayers = 6;
	const char* maxPlayersVar = GetCurrentPlaylistVar("max_players", false);
	if (maxPlayersVar) // GetCurrentPlaylistVar can return null so protect against this
		maxPlayers = std::stoi(maxPlayersVar);

	// net_data_block_enabled is required for sp, force it if we're on an sp map
	// sucks for security but just how it be
	if (!strncmp(g_pHostState->m_levelName, "sp_", 3))
		g_pCVar->FindVar("net_data_block_enabled")->SetValue(true);

	g_MasterServerManager->UpdateServerMapAndPlaylist(hostState->m_levelName, GetCurrentPlaylistName(), maxPlayers);

	double dStartTime = Tier0::Plat_FloatTime();
	CHostState__State_ChangeLevelMP(hostState);
	spdlog::info("loading took {}s", Tier0::Plat_FloatTime() - dStartTime);
}

typedef void (*CHostState__State_GameShutdownType)(CHostState* hostState);
CHostState__State_GameShutdownType CHostState__State_GameShutdown;
void CHostState__State_GameShutdownHook(CHostState* hostState)
{
	spdlog::info("HostState: GameShutdown");

	g_MasterServerManager->RemoveSelfFromServerList();
	g_ServerAuthenticationManager->StopPlayerAuthServer();

	CHostState__State_GameShutdown(hostState);
}

ON_DLL_LOAD_RELIESON("engine.dll", HostState, ConVar, [](HMODULE baseAddress)
{
	g_pHostState = (CHostState*)((char*)baseAddress + 0x7CF180);

	HookEnabler hook;
	ENABLER_CREATEHOOK(
		hook, (char*)baseAddress + 0x16E7D0, CHostState__State_NewGameHook, reinterpret_cast<LPVOID*>(&CHostState__State_NewGame));
	ENABLER_CREATEHOOK(
		hook, (char*)baseAddress + 0x16E520, CHostState__State_ChangeLevelMPHook, reinterpret_cast<LPVOID*>(&CHostState__State_ChangeLevelMP));
	ENABLER_CREATEHOOK(
		hook, (char*)baseAddress + 0x16E640, CHostState__State_GameShutdownHook, reinterpret_cast<LPVOID*>(&CHostState__State_GameShutdown));
})