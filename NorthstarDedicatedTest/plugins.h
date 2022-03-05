#pragma once
#include "plugin_abi.h"

int getServerInfoChar(char* out_buf, size_t out_buf_len, ServerInfoType var);
int getServerInfoInt(int* out_ptr, ServerInfoType var);
int getServerInfoBool(bool* out_ptr, ServerInfoType var);

int getGameStateChar(char* out_buf, size_t out_buf_len, GameStateInfoType var);
int getGameStateInt(int* out_ptr, GameStateInfoType var);
int getGameStateBool(bool* out_ptr, GameStateInfoType var);

int getPlayerInfoChar(char* out_buf, size_t out_buf_len, PlayerInfoType var);
int getPlayerInfoInt(int* out_ptr, PlayerInfoType var);
int getPlayerInfoBool(bool* out_ptr, PlayerInfoType var);

void initGameState();
void* getPluginObject(PluginObject var);

struct InternalGameState
{
	int ourScore;
	int secondHighestScore;
	int highestScore;

	bool connected;
	bool loading;
	std::string map;
	std::string mapDisplayName;
	std::string playlist;
	std::string playlistDisplayName;
	int players;
};
struct InternalServerInfo
{
	std::string id;
	std::string name;
	std::string description;
	std::string password;
	int maxPlayers;
	bool roundBased;
	int scoreLimit;
	int endTime;
};
// TODO: need to extend this to include current player data like loadouts
struct InternalPlayerInfo
{
	int uid;
};

extern InternalGameState gameState;
extern InternalServerInfo serverInfo;
extern InternalPlayerInfo playerInfo;

void InitialisePluginCommands(HMODULE baseAddress);