#include "pch.h"
#include "pluginbackend.h"
#include "plugin_abi.h"
#include "serverpresence.h"
#include "masterserver.h"
#include "squirrel.h"
#include "plugins.h"

#include "concommand.h"

#define EXPORT extern "C" __declspec(dllexport)

AUTOHOOK_INIT()

PluginCommunicationHandler* g_pPluginCommunicationhandler;

static PluginDataRequest storedRequest {PluginDataRequestType::END, (PluginRespondDataCallable) nullptr};

void init_plugincommunicationhandler()
{
	g_pPluginCommunicationhandler = new PluginCommunicationHandler;
	g_pPluginCommunicationhandler->requestQueue = {};
}

void PluginCommunicationHandler::RunFrame()
{
	std::lock_guard<std::mutex> lock(requestMutex);
	if (!requestQueue.empty())
	{
		storedRequest = requestQueue.front();
		switch (storedRequest.type)
		{
		default:
			spdlog::error("{} was called with invalid request type '{}'", __FUNCTION__, static_cast<int>(storedRequest.type));
		}
		requestQueue.pop();
	}
}

void PluginCommunicationHandler::PushRequest(PluginDataRequestType type, PluginRespondDataCallable func)
{
	std::lock_guard<std::mutex> lock(requestMutex);
	requestQueue.push(PluginDataRequest {type, func});
}

void PluginCommunicationHandler::GeneratePresenceObjects()
{
	PluginGameStatePresence presence {};

	presence.id = g_pGameStatePresence->id.c_str();
	presence.name = g_pGameStatePresence->name.c_str();
	presence.description = g_pGameStatePresence->description.c_str();
	presence.password = g_pGameStatePresence->password.c_str();

	presence.is_server = g_pGameStatePresence->is_server;
	presence.is_local = g_pGameStatePresence->is_local;

	if (g_pGameStatePresence->is_loading)
		presence.state = GameState::LOADING;
	else if (g_pGameStatePresence->ui_map == "")
		presence.state = GameState::MAINMENU;
	else if (g_pGameStatePresence->map == "mp_lobby" && g_pGameStatePresence->is_local && g_pGameStatePresence->is_lobby)
		presence.state = GameState::LOBBY;
	else
		presence.state = GameState::INGAME;

	presence.map = g_pGameStatePresence->map.c_str();
	presence.map_displayname = g_pGameStatePresence->map_displayname.c_str();
	presence.playlist = g_pGameStatePresence->playlist.c_str();
	presence.playlist_displayname = g_pGameStatePresence->playlist_displayname.c_str();

	presence.current_players = g_pGameStatePresence->current_players;
	presence.max_players = g_pGameStatePresence->max_players;
	presence.own_score = g_pGameStatePresence->own_score;
	presence.other_highest_score = g_pGameStatePresence->other_highest_score;
	presence.max_score = g_pGameStatePresence->max_score;
	presence.timestamp_end = g_pGameStatePresence->timestamp_end;
	g_pPluginManager->PushPresence(&presence);
}

ON_DLL_LOAD_RELIESON("engine.dll", PluginBackendEngine, ConCommand, (CModule module))
{
	g_pPluginManager->InformDLLLoad(PluginLoadDLL::ENGINE, &g_pPluginCommunicationhandler->m_sEngineData);
}

ON_DLL_LOAD_RELIESON("client.dll", PluginBackendClient, ConCommand, (CModule module))
{
	g_pPluginManager->InformDLLLoad(PluginLoadDLL::CLIENT, nullptr);
}


ON_DLL_LOAD_RELIESON("server.dll", PluginBackendServer, ConCommand, (CModule module))
{
	g_pPluginManager->InformDLLLoad(PluginLoadDLL::SERVER, nullptr);
}
