#pragma once
#include "core/convar/convar.h"
#include "engine/r2engine.h"

class ServerBotManager
{

  public:
	ConVar* Cvar_bot_quota;
	ConVar* Cvar_bot_teams;

	ConVar* Cvar_bot_clantag;

	ConVar* Cvar_bot_pilot_settings;
	ConVar* Cvar_bot_force_pilot_primary;
	ConVar* Cvar_bot_force_pilot_secondary;
	ConVar* Cvar_bot_force_pilot_weapon3;
	ConVar* Cvar_bot_force_pilot_ordnance;
	ConVar* Cvar_bot_force_pilot_ability;

	ConVar* Cvar_bot_titan_settings;
	ConVar* Cvar_bot_force_titan_ordnance;
	ConVar* Cvar_bot_force_titan_ability;

  public:
	ServerBotManager();

	void AddBot(const char* pName = "", int iTeam = -1);

	// events we react to
	void StartMatch();
	void SetupPlayer(R2::CBaseClient* pPlayer);
	void SimulatePlayers();

};

extern ServerBotManager* g_pBots;
