#include "pch.h"
#include "bots.h"

#include "core/convar/concommand.h"
#include "engine/r2engine.h"
#include "shared/maxplayers.h"
#include "shared/playlist.h"
#include "server/auth/serverauthentication.h"
#include "server/r2server.h"

AUTOHOOK_INIT()

ServerBotManager* g_pBots;

void* pServer;
R2::CBaseClient* (*__fastcall CBaseServer__CreateFakeClient)(
	void* self, const char* pName, const char* pUnk, const char* pDesiredPlaylist, int nDesiredTeam);

void* pServerGameClients;
void (*__fastcall ServerGameClients__ClientFullyConnect)(void* self, uint16_t edict, bool bRestore);

void (*__fastcall CBasePlayer__RunNullCommand)(R2::CBasePlayer* self);

// maybe this could be defined in a cfg instead at some point?
const std::vector<const char*> BOT_NAMES = {
	// normal titanfall names
	"Jack",
	"Cooper",
	"BT-7274",
	"FS-1041",
	"Blisk",
	"Marv",
	"Marvin",
	"Barker",
	"Gates",
	"Marder",
	"Sarah",
	"Briggs",
	"Viper",
	"Ash",
	"Richter",
	"Slone",
	"Tai",
	"Lastimosa",
	"Davis",
	"Droz",
	"Bear",
	"MacAllan",
	"Bish",
	"Spyglass",

	// tf1 training/campaign npc names
	"Rakowski",
	"Alavi",
	"Riggs",
	"Bracken",
	"Hainey",
	"Vaughan",
	"Dunnam",
	"Riggins",
	"Messerly",
	"Heppe",
	"Keating",
	"Blanton",
	"Emslie",
	"Flagg",
	"Hofner",

	// deepcut references lol
	"Iron",
	"Starseeker",
	"Hollow",
	"Preacher",

	// contributors/cool people
	"BobTheBot",
	"GeckoEidechsBot",
	"TaskiBot",
	"RoyalBot",
	"EmmaB(ot)", // ok this one is shit to be honest but it's a hard name to put bot into
	"bot0358",
	// how the fuck do i turn amos into a bot name oh my god
	"Botmos",
	"Botscuit",
	"Botty_RaVen",

	"deez_bots", // this is really funny ok
};

// this is the root func from what usercmd running is called from
AUTOHOOK(SomeRunUsercmdFunc, server.dll + 0x483A50,
void, __fastcall, (char a1))
{
	g_pBots->SimulatePlayers();
	SomeRunUsercmdFunc(a1);
}

void ConCommand_bot_add(const CCommand& arg)
{		
	if (arg.ArgC() == 2)
		g_pBots->AddBot(arg.Arg(1));
	else if (arg.ArgC() == 3)
		g_pBots->AddBot(arg.Arg(1), std::stoi(arg.Arg(2)));
	else
		g_pBots->AddBot();
}

ServerBotManager::ServerBotManager()
{
	//m_Cvar_bot_teams = new ConVar("")
}


// add a new bot player
void ServerBotManager::AddBot(const char* pName, int iTeam)
{
	if (!*pName)
	{
		for (int i = 0; i < R2::GetMaxPlayers(); i++)
		{
			R2::CBaseClient* pClient = &R2::g_pClientArray[i];
			if (pClient->m_Signon >= R2::eSignonState::CONNECTED)
			{
			}
		}
	}
		pName = BOT_NAMES[rand() % BOT_NAMES.size()];

	const bool bCheckTeams = iTeam == -1;
	const int nMaxPlayers = std::stoi(R2::GetCurrentPlaylistVar("max_players", true));

	int nPlayers = 0;

	int nImcPlayers = 0;
	int nMilitiaPlayers = 0;

	// NOTE: doesn't currently support ffa teams
	for (int i = 0; i < R2::GetMaxPlayers(); i++)
	{
		R2::CBaseClient* pClient = &R2::g_pClientArray[i];
		if (pClient->m_Signon >= R2::eSignonState::CONNECTED)
			nPlayers++;

		R2::CBasePlayer* pPlayer = R2::UTIL_PlayerByIndex(i + 1);
		if (!pPlayer)
			continue;

		if (bCheckTeams)
		{
			if (pPlayer->m_nTeam == 2)
				nImcPlayers++;
			else
				nMilitiaPlayers++;
		}
	}

	if (nPlayers >= nMaxPlayers)
	{
		spdlog::warn("tried to spawn more bots than the server has slots available!");
		return;
	}

	if (bCheckTeams)
	{
		// pick team that needs players
		if (nImcPlayers > nMilitiaPlayers)
			iTeam = 3;
		else
			iTeam = 2;
	}

	const R2::CBaseClient* pPlayer = CBaseServer__CreateFakeClient(pServer, pName, "", "", iTeam);
	ServerGameClients__ClientFullyConnect(pServerGameClients, pPlayer->edict, false); // this is normally done on eSignonState::FULL, but bots don't properly call it

	spdlog::info("Created bot {}", pPlayer->m_Name);
}


// add bots at the start of a game
void ServerBotManager::StartMatch()
{
	UpdateBotCounts();
}

void ServerBotManager::UpdateBotCounts()
{
	int nNumBots = 0;
	int nNumHumans = 0;

	for (int i = 0; i < R2::GetMaxPlayers(); i++)
	{
		R2::CBaseClient* pClient = &R2::g_pClientArray[i];
		if (pClient->m_Signon >= R2::eSignonState::CONNECTED)
		{
			if (pClient->m_bFakePlayer)
				nNumBots++;
			else
				nNumHumans++;
		}
	}

	// ignore specified number of humans unless convar = -1
	if (Cvar_bot_quota_ignorehumans->GetInt() >= 0)
		nNumHumans = std::max(0, nNumHumans - Cvar_bot_quota_ignorehumans->GetInt());
	else
		nNumHumans = 0;

	int nRequiredBots = Cvar_bot_quota->GetInt() - nNumBots - nNumHumans;
	if (nRequiredBots >= 0)
	{
		for (int i = nRequiredBots; i > 0; i--)
			AddBot();
	}
	else
	{
		// remove bots
		//for (int i = nRequiredBots; i < 0; i++)
		
	}
}

// setup bot after it connects
void ServerBotManager::SetupPlayer(R2::CBaseClient* pPlayer)
{
	strncpy_s(pPlayer->m_ClanTag, Cvar_bot_clantag->GetString(), 15);
}

// every server frame, run bot usercmds
void ServerBotManager::SimulatePlayers()
{
	if (!Cvar_bot_simulate->GetBool())
		return;

	for (int i = 0; i < R2::GetMaxPlayers(); i++)
	{
		R2::CBaseClient* pClient = &R2::g_pClientArray[i];
		if (pClient->m_Signon == R2::eSignonState::FULL && pClient->m_bFakePlayer)
		{
			// todo: if we ever want actual moving bots, this would be where we'd nav, build usercmd, etc
			CBasePlayer__RunNullCommand(R2::UTIL_PlayerByIndex(i + 1));
		}
	}
}

ON_DLL_LOAD_RELIESON("engine.dll", Bots, (ConVar, ConCommand), (CModule module))
{
	g_pBots = new ServerBotManager;
	g_pBots->Cvar_bot_quota = new ConVar("bot_quota", "0", FCVAR_GAMEDLL, "Minimum number of players when filling game with bots");
	g_pBots->Cvar_bot_quota_ignorehumans =
		new ConVar("bot_quota_ignorehumans", "0", FCVAR_GAMEDLL, "Number of human players to ignore for bot_quota, -1 to ignore all (for ignoring spectator players)");
	g_pBots->Cvar_bot_clantag = new ConVar("bot_clantag", "BOT", FCVAR_GAMEDLL, "Clantag to give bots");
	g_pBots->Cvar_bot_simulate = new ConVar("bot_simulate", "1", FCVAR_GAMEDLL, "Whether to run bot movement at all");

	g_pBots->Cvar_bot_pilot_settings = new ConVar("bot_pilot_settings", "", FCVAR_GAMEDLL, "force pilot playersettings for bots");
	g_pBots->Cvar_bot_force_pilot_primary = new ConVar("bot_force_pilot_primary", "", FCVAR_GAMEDLL, "force pilot primary weapon for bots");
	g_pBots->Cvar_bot_force_pilot_secondary = new ConVar("bot_force_pilot_secondary", "", FCVAR_GAMEDLL, "force pilot secondary weapon for bots");
	g_pBots->Cvar_bot_force_pilot_weapon3 = new ConVar("bot_force_pilot_weapon3", "", FCVAR_GAMEDLL, "force pilot 3rd weapon for bots");
	g_pBots->Cvar_bot_force_pilot_ordnance = new ConVar("bot_force_pilot_ordnance", "", FCVAR_GAMEDLL, "force pilot ordnance for bots");
	g_pBots->Cvar_bot_force_pilot_ability = new ConVar("bot_force_pilot_ability", "", FCVAR_GAMEDLL, "force pilot ability for bots");

	g_pBots->Cvar_bot_titan_settings = new ConVar("bot_titan_settings", "", FCVAR_GAMEDLL, "force titan playersettings for bots");
	g_pBots->Cvar_bot_force_titan_ordnance = new ConVar("bot_force_titan_ordnance", "", FCVAR_GAMEDLL, "force titan ordnance for bots");
	g_pBots->Cvar_bot_force_titan_ability = new ConVar("bot_force_titan_ability", "", FCVAR_GAMEDLL, "force titan ability for bots");

	pServer = module.Offset(0x12A53D40).As<void*>();
	pServerGameClients = module.Offset(0x13F0AAA8).As<void*>();

	CBaseServer__CreateFakeClient =
		module.Offset(0x114C60).As<R2::CBaseClient* (*__fastcall)(void*, const char*, const char*, const char*, int)>();

	RegisterConCommand(
		"bot_add",
		ConCommand_bot_add,
		"Add a bot, by default name and team are automatically chosen, they can be specified (bot_add name team)",
		FCVAR_GAMEDLL);
}

ON_DLL_LOAD("server.dll", ServerBots, (CModule module))
{
	AUTOHOOK_DISPATCH_MODULE(server.dll)

	ServerGameClients__ClientFullyConnect = module.Offset(0x153B70).As<void (*__fastcall)(void*, uint16_t, bool)>();
	CBasePlayer__RunNullCommand = module.Offset(0x5A9FD0).As<void (*__fastcall)(R2::CBasePlayer*)>();
}
