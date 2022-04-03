#pragma once
#include <string>

#define ABI_VERSION 1
/// <summary>
/// This enum is used for referencing the different types of objects we can pass to a plugin
/// Anything exposed to a plugin must not a be C++ type, as they could break when compiling with a different compiler.
/// Any ABI incompatible change must increment the version number.
/// Nothing must be removed from this enum, only appended. When it absolutely necessary to deprecate an object, it should return UNSUPPORTED
/// when retrieved
/// </summary>
enum PluginObject
{
	UNSUPPORTED = 0,
	GAMESTATE = 1,
	SERVERINFO = 2,
	PLAYERINFO = 3,
	DUMMY = 0xFFFF
};

enum GameStateInfoType
{
	ourScore = 0,
	secondHighestScore = 1,
	highestScore = 2,
	connected = 3,
	loading = 4,
	map = 5,
	mapDisplayName = 6,
	playlist = 7,
	playlistDisplayName = 8,
	players = 9
};
struct GameState
{
	int (*fnGetGameStateChar)(char* out_buf, size_t out_buf_len, GameStateInfoType var);
	int (*fnGetGameStateInt)(int* out_ptr, GameStateInfoType var);
	int (*fnGetGameStateBool)(bool* out_ptr, GameStateInfoType var);
};

enum ServerInfoType
{
	id = 0,
	name = 1,
	description = 2,
	password = 3,
	maxPlayers = 4,
	roundBased = 5,
	scoreLimit = 6,
	endTime = 7
};
struct ServerInfo
{
	int (*fnGetServerInfoChar)(char* out_buf, size_t out_buf_len, ServerInfoType var);
	int (*fnGetServerInfoInt)(int* out_ptr, ServerInfoType var);
	int (*fnGetServerInfoBool)(bool* out_ptr, ServerInfoType var);
};

enum PlayerInfoType
{
	uid = 0
};
struct PlayerInfo
{
	int (*fnGetPlayerInfoChar)(char* out_buf, size_t out_buf_len, PlayerInfoType var);
	int (*fnGetPlayerInfoInt)(int* out_ptr, PlayerInfoType var);
	int (*fnGetPlayerInfoBool)(bool* out_ptr, PlayerInfoType var);
};