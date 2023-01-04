#pragma once
#include "convar.h"
#include "r2engine.h"

class ServerBotManager
{

  public:
	ConVar* Cvar_bot_quota;
	ConVar* Cvar_bot_teams;

	ConVar* Cvar_bot_clantag;

  public:
	ServerBotManager();

	void AddBot(const char* pName = "", int iTeam = -1);

	// events we react to
	void StartMatch();
	void SetupPlayer(R2::CBaseClient* pPlayer);
	void SimulatePlayers();

};

extern ServerBotManager* g_pBots;
