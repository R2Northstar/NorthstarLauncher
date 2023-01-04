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
	if (arg.ArgC() == 1)
		g_pBots->AddBot();
	else if (arg.ArgC() == 2)
		g_pBots->AddBot(arg.Arg(1));
	else if (arg.ArgC() == 2)
		g_pBots->AddBot(arg.Arg(1), std::stoi(arg.Arg(2)));
}

ServerBotManager::ServerBotManager()
{
	//m_Cvar_bot_teams = new ConVar("")
}


// add a new bot player
void ServerBotManager::AddBot(const char* pName, int iTeam)
{
	if (!*pName)
		pName = BOT_NAMES[rand() % BOT_NAMES.size()];

	if (iTeam == -1)
	{
		// pick team that needs players
		// oops i cant be bothered just do imc for now
		iTeam = 2;
	}

	const R2::CBaseClient* pPlayer = CBaseServer__CreateFakeClient(pServer, pName, "", "", iTeam);
	ServerGameClients__ClientFullyConnect(pServerGameClients, pPlayer->edict, false); // this is normally done on eSignonState::FULL, but bots don't properly call it

	spdlog::info("Created bot {}", pPlayer->m_Name);
}


// add bots at the start of a game
void ServerBotManager::StartMatch()
{
	int nNumBots = Cvar_bot_quota->GetInt() - g_pServerAuthentication->m_PlayerAuthenticationData.size();
	for (int i = nNumBots; i > 0; i--)
		AddBot();
}

// setup bot after it connects
void ServerBotManager::SetupPlayer(R2::CBaseClient* pPlayer)
{
	strncpy_s(pPlayer->m_ClanTag, Cvar_bot_clantag->GetString(), 15);
}

// every server frame, run bot usercmds
void ServerBotManager::SimulatePlayers()
{
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
	g_pBots->Cvar_bot_clantag = new ConVar("bot_clantag", "BOT", FCVAR_GAMEDLL, "Clantag to give bots");

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
