#include "pch.h"

#include "gamepresence.h"
#include "pluginbackend.h"
#include "plugins.h"
#include "dedicated.h"
#include "serverpresence.h"

GameStatePresence* g_pGameStatePresence;

GameStatePresence::GameStatePresence()
{
	g_pServerPresence->AddPresenceReporter(&m_GameStateServerPresenceReporter);
}

void GameStateServerPresenceReporter::RunFrame(double flCurrentTime, const ServerPresence* pServerPresence)
{
	g_pGameStatePresence->id = pServerPresence->m_sServerId;
	g_pGameStatePresence->name = pServerPresence->m_sServerName;
	g_pGameStatePresence->description = pServerPresence->m_sServerDesc;
	g_pGameStatePresence->password = pServerPresence->m_Password;

	g_pGameStatePresence->map = pServerPresence->m_MapName;
	g_pGameStatePresence->playlist = pServerPresence->m_PlaylistName;
	g_pGameStatePresence->current_players = pServerPresence->m_iPlayerCount;
	g_pGameStatePresence->max_players = pServerPresence->m_iMaxPlayers;

	g_pGameStatePresence->is_local = !IsDedicatedServer();
}

void GameStatePresence::RunFrame()
{
	auto test = g_pSquirrel<ScriptContext::UI>;
	if (g_pSquirrel<ScriptContext::UI>->m_pSQVM != nullptr && g_pSquirrel<ScriptContext::UI>->m_pSQVM->sqvm != nullptr)
		g_pSquirrel<ScriptContext::UI>->Call("GenerateUIPresence");

	if (g_pSquirrel<ScriptContext::CLIENT>->m_pSQVM != nullptr && g_pSquirrel<ScriptContext::CLIENT>->m_pSQVM->sqvm != nullptr)
	{
		auto test = g_pSquirrel<ScriptContext::CLIENT>->Call("GenerateGameState");
	}
	g_pPluginCommunicationhandler->GeneratePresenceObjects();
}

ADD_SQFUNC("void", NSPushGameStateData, "struct gamestate", "", ScriptContext::CLIENT)
{
	SQStructInstance* structInst = g_pSquirrel<ScriptContext::CLIENT>->m_pSQVM->sqvm->_stackOfCurrentFunction[1]._VAL.asStructInstance;
	g_pGameStatePresence->map = structInst->data[0]._VAL.asString->_val;
	g_pGameStatePresence->map_displayname = structInst->data[1]._VAL.asString->_val;
	g_pGameStatePresence->playlist = structInst->data[2]._VAL.asString->_val;
	g_pGameStatePresence->playlist_displayname = structInst->data[3]._VAL.asString->_val;

	g_pGameStatePresence->current_players = structInst->data[4]._VAL.asInteger;
	g_pGameStatePresence->max_players = structInst->data[5]._VAL.asInteger;
	g_pGameStatePresence->own_score = structInst->data[6]._VAL.asInteger;
	g_pGameStatePresence->other_highest_score = structInst->data[7]._VAL.asInteger;
	g_pGameStatePresence->max_score = structInst->data[8]._VAL.asInteger;
	g_pGameStatePresence->timestamp_end = ceil(structInst->data[9]._VAL.asFloat);

	return SQRESULT_NOTNULL;
}

ADD_SQFUNC("void", NSPushUIPresence, "struct presence", "", ScriptContext::UI)
{
	SQStructInstance* structInst = g_pSquirrel<ScriptContext::UI>->m_pSQVM->sqvm->_stackOfCurrentFunction[1]._VAL.asStructInstance;

	g_pGameStatePresence->is_loading = structInst->data[0]._VAL.asInteger;
	g_pGameStatePresence->is_lobby = structInst->data[1]._VAL.asInteger;
	g_pGameStatePresence->loading_level = structInst->data[2]._VAL.asString->_val;
	g_pGameStatePresence->ui_map = structInst->data[3]._VAL.asString->_val;

	return SQRESULT_NOTNULL;
}
