#pragma once
#include "r2server.h"
#include "convar.h"
#include <unordered_map>

struct PlayerLimitData
{
	double lastClientCommandQuotaStart = -1.0;
	int numClientCommandsInQuota = 0;

	double lastNetChanProcessingLimitStart = -1.0;
	double netChanProcessingLimitTime = 0.0;

	double lastSayTextLimitStart = -1.0;
	int sayTextLimitCount = 0;
};

struct UnconnectedPlayerLimitData
{
	char ip[16];
	double lastQuotaStart = 0.0;
	int packetCount = 0;
	double timeoutEnd = -1.0;
};

class ServerLimitsManager
{
  public:
	ConVar* CVar_sv_quota_stringcmdspersecond;
	ConVar* Cvar_net_chan_limit_mode;
	ConVar* Cvar_net_chan_limit_msec_per_sec;
	ConVar* Cvar_sv_querylimit_per_sec;
	ConVar* Cvar_sv_max_chat_messages_per_sec;

	std::unordered_map<R2::CBasePlayer*, PlayerLimitData> m_PlayerLimitData;
	std::vector<UnconnectedPlayerLimitData> m_UnconnectedPlayerLimitData;

  public:
	void AddPlayer(R2::CBasePlayer* player);
	bool CheckStringCommandLimits(R2::CBasePlayer* player);
	bool CheckChatLimits(R2::CBasePlayer* player);
};

extern ServerLimitsManager* g_pServerLimits;
