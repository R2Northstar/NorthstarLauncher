#pragma once
#include <string>

struct ServerInfo
{
	std::string id;
	std::string name;
	std::string description;
	std::string password;
	int maxPlayers;
	bool roundBased;
	int scoreLimit;
};

struct PlayerInfo
{
	int uid;
	// stuff like current weapons ig?
};

struct GameState
{
	ServerInfo serverInfo;
	PlayerInfo playerInfo;

	int ourScore;
	int secondHighestScore;
	int highestScore;

	bool connected;
	std::string map;
	std::string mapDisplayName;
	std::string playlist;
	std::string playlistDisplayName;
	int players;
};