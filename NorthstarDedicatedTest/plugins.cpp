#include "pch.h"
#include "squirrel.h"
#include "state.h"
#include "plugins.h"
#include <chrono>
#include "masterserver.h"

GameState gameState;
ServerInfo serverInfo;
PlayerInfo playerInfo;


void initGameState()
{
	serverInfo.id = "";
	serverInfo.name = "";
	serverInfo.description = "";
	serverInfo.password = "";
	serverInfo.maxPlayers = 0;
	gameState.connected = false;
	gameState.loading = false;
	gameState.map = "";
	gameState.mapDisplayName = "";
	gameState.playlist = "";
	gameState.playlistDisplayName = "";
	gameState.players = 0;

	playerInfo.uid = 123;
}

SQRESULT SQ_UpdateGameStateUI(void* sqvm)
{
	gameState.map = ClientSq_getstring(sqvm, 1);
	gameState.mapDisplayName = ClientSq_getstring(sqvm, 2);
	gameState.playlist = ClientSq_getstring(sqvm, 3);
	gameState.playlistDisplayName = ClientSq_getstring(sqvm, 4);
	gameState.connected = ClientSq_getbool(sqvm, 5);
	gameState.loading = ClientSq_getbool(sqvm, 6);
	return SQRESULT_NOTNULL;
}

SQRESULT SQ_UpdateGameStateClient(void* sqvm)
{
	gameState.players = ClientSq_getinteger(sqvm, 1);
	gameState.ourScore = ClientSq_getinteger(sqvm, 2);
	gameState.secondHighestScore = ClientSq_getinteger(sqvm, 3);
	gameState.highestScore = ClientSq_getinteger(sqvm, 4);
	gameState.serverInfo.roundBased = ClientSq_getbool(sqvm, 5);
	gameState.serverInfo.scoreLimit = ClientSq_getbool(sqvm, 6);
	return SQRESULT_NOTNULL;
}

// string id, string name, string password, int player, int maxPlayers, 
// string map, string mapDisplayName, string playlist, string playlistDisplayName
SQRESULT SQ_UpdateServerInfo(void* sqvm)
{
	spdlog::info("=============================SERVER INFO UPDATE SUCCESFUL=============================");
	gameState.serverInfo.id = ClientSq_getstring(sqvm, 1);
	gameState.serverInfo.name = ClientSq_getstring(sqvm, 2);
	gameState.serverInfo.password = ClientSq_getstring(sqvm, 3);
	gameState.players = ClientSq_getinteger(sqvm, 4);
	gameState.serverInfo.maxPlayers = ClientSq_getinteger(sqvm, 5);
	gameState.map = ClientSq_getstring(sqvm, 6);
	gameState.mapDisplayName = ClientSq_getstring(sqvm, 7);
	gameState.playlist = ClientSq_getstring(sqvm, 8);
	gameState.playlistDisplayName = ClientSq_getstring(sqvm, 9);
	return SQRESULT_NOTNULL;
}

SQRESULT SQ_UpdateServerInfoBetweenRounds(void* sqvm)
{
	gameState.serverInfo.id = ClientSq_getstring(sqvm, 1);
	gameState.serverInfo.name = ClientSq_getstring(sqvm, 2);
	gameState.serverInfo.password = ClientSq_getstring(sqvm, 3);
	gameState.serverInfo.maxPlayers = ClientSq_getinteger(sqvm, 4);
	return SQRESULT_NOTNULL;
}


SQRESULT SQ_UpdateTimeInfo(void* sqvm)
{
	int endTimeFromNow = ceil(ClientSq_getfloat(sqvm, 1));
	const auto p1 = std::chrono::system_clock::now().time_since_epoch();
	gameState.serverInfo.endTime = std::chrono::duration_cast<std::chrono::seconds>(p1).count() + endTimeFromNow;
	return SQRESULT_NOTNULL;
}

SQRESULT SQ_SetConnected(void* sqvm)
{
	gameState.loading = ClientSq_getbool(sqvm, 1);
	return SQRESULT_NOTNULL;
}


SQRESULT SQ_UpdateListenServer(void* sqvm)
{
	gameState.serverInfo.id = g_MasterServerManager->m_ownServerId;
	gameState.serverInfo.password = FindConVar("ns_server_password")->m_pszString;
	return SQRESULT_NOTNULL;
}





void InitialisePluginCommands(HMODULE baseAddress)
{
	// i swear there's a way to make this not have be run in 2 contexts but i can't figure it out
	// some funcs i need are just not available in UI or CLIENT

	//g_UISquirrelManager->AddFuncRegistration("void", "NSUpdateServerInfo", "", "", SQ_UpdateGameState);
	//g_UISquirrelManager->AddFuncRegistration("void", "NSUpdatePlayerInfo", "", "", SQ_UpdateGameState);
	
	g_UISquirrelManager->AddFuncRegistration("void", "NSUpdateGameStateUI", "string gamemode, string gamemodeName, string map, string mapName, bool connected, bool loading", "", SQ_UpdateGameStateUI);
	g_ClientSquirrelManager->AddFuncRegistration("void", "NSUpdateGameStateClient", "int playerCount, int outScore, int secondHighestScore, int highestScore, bool roundBased, int scoreLimit", "", SQ_UpdateGameStateClient);
	g_UISquirrelManager->AddFuncRegistration(
		"void",
		"NSUpdateServerInfo", "string id, string name, string password, int players, int maxPlayers, string map, string mapDisplayName, string playlist, string playlistDisplayName",
		"", SQ_UpdateServerInfo);
	g_ClientSquirrelManager->AddFuncRegistration("void", "NSUpdateServerInfoReload", "int maxPlayers", "", SQ_UpdateServerInfoBetweenRounds);
	g_ClientSquirrelManager->AddFuncRegistration("void", "NSUpdateTimeInfo", "float timeInFuture", "", SQ_UpdateTimeInfo);
	g_UISquirrelManager->AddFuncRegistration("void", "NSSetLoading", "bool loading", "", SQ_SetConnected);
	g_UISquirrelManager->AddFuncRegistration("void", "NSUpdateListenServer", "", "", SQ_UpdateListenServer);
}
