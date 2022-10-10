#pragma once
#include "convar.h"
#include "serverpresence.h"
#include <winsock2.h>
#include <string>
#include <cstring>

extern ConVar* Cvar_ns_masterserver_hostname;
extern ConVar* Cvar_ns_curl_log_enable;

struct RemoteModInfo
{
  public:
	std::string Name;
	std::string Version;
};

class RemoteServerInfo
{
  public:
	char id[33]; // 32 bytes + nullterminator

	// server info
	char name[64];
	std::string description;
	char map[32];
	char playlist[16];
	std::vector<RemoteModInfo> requiredMods;

	int playerCount;
	int maxPlayers;

	// connection stuff
	bool requiresPassword;

  public:
	RemoteServerInfo(
		const char* newId,
		const char* newName,
		const char* newDescription,
		const char* newMap,
		const char* newPlaylist,
		int newPlayerCount,
		int newMaxPlayers,
		bool newRequiresPassword);
};

struct RemoteServerConnectionInfo
{
  public:
	char authToken[32];

	in_addr ip;
	unsigned short port;
};

struct MainMenuPromoData
{
  public:
	std::string newInfoTitle1;
	std::string newInfoTitle2;
	std::string newInfoTitle3;

	std::string largeButtonTitle;
	std::string largeButtonText;
	std::string largeButtonUrl;
	int largeButtonImageIndex;

	std::string smallButton1Title;
	std::string smallButton1Url;
	int smallButton1ImageIndex;

	std::string smallButton2Title;
	std::string smallButton2Url;
	int smallButton2ImageIndex;
};

class MasterServerManager
{
  private:
	bool m_bRequestingServerList = false;
	bool m_bAuthenticatingWithGameServer = false;

  public:
	char m_sOwnServerId[33];
	char m_sOwnServerAuthToken[33];
	char m_sOwnClientAuthToken[33];

	std::string m_sOwnModInfoJson;

	bool m_bOriginAuthWithMasterServerDone = false;
	bool m_bOriginAuthWithMasterServerInProgress = false;

	bool m_bRequireClientAuth = false;
	bool m_bSavingPersistentData = false;

	bool m_bScriptRequestingServerList = false;
	bool m_bSuccessfullyConnected = true;

	bool m_bNewgameAfterSelfAuth = false;
	bool m_bScriptAuthenticatingWithGameServer = false;
	bool m_bSuccessfullyAuthenticatedWithGameServer = false;
	std::string m_sAuthFailureReason {};

	bool m_bHasPendingConnectionInfo = false;
	RemoteServerConnectionInfo m_pendingConnectionInfo;
	std::optional<RemoteServerInfo> m_currentServer;
	std::string m_sCurrentServerPassword;

	std::vector<RemoteServerInfo> m_vRemoteServers;

	bool m_bHasMainMenuPromoData = false;
	MainMenuPromoData m_sMainMenuPromoData;

  public:
	MasterServerManager();

	void ClearServerList();
	void RequestServerList();
	void RequestMainMenuPromos();
	void AuthenticateOriginWithMasterServer(const char* uid, const char* originToken);
	void AuthenticateWithOwnServer(const char* uid, const char* playerToken);
	void AuthenticateWithServer(const char* uid, const char* playerToken, RemoteServerInfo server, const char* password);
	void WritePlayerPersistentData(const char* playerId, const char* pdata, size_t pdataSize);
};

extern MasterServerManager* g_pMasterServerManager;
extern ConVar* Cvar_ns_masterserver_hostname;
