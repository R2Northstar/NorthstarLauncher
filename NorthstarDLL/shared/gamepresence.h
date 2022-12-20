
#pragma once
#include "pch.h"
#include "server/serverpresence.h"

class GameStateServerPresenceReporter : public ServerPresenceReporter
{
	void RunFrame(double flCurrentTime, const ServerPresence* pServerPresence);
};

class GameStatePresence
{
  public:
	std::string id;
	std::string name;
	std::string description;
	std::string password; // NOTE: May be empty

	bool is_server;
	bool is_local = false;
	bool is_loading;
	bool is_lobby;
	std::string loading_level;

	std::string ui_map;

	std::string map;
	std::string map_displayname;
	std::string playlist;
	std::string playlist_displayname;

	int current_players;
	int max_players;

	int own_score;
	int other_highest_score; // NOTE: The highest score OR the second highest score if we have the highest
	int max_score;

	int timestamp_end;

	GameStatePresence();
	void RunFrame();

  protected:
	GameStateServerPresenceReporter m_GameStateServerPresenceReporter;
};

extern GameStatePresence* g_pGameStatePresence;
