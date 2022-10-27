#pragma once
#include "pch.h"
#include "plugin_abi.h"

#include <queue>
#include <mutex>

enum PluginDataRequestType
{
	SERVER = 0,
	GAMESTATE = 1,
	RPC = 2
};

union PluginRespondDataCallable
{
	PLUGIN_RESPOND_SERVER_DATA_TYPE asServerData;
	PLUGIN_RESPOND_GAMESTATE_DATA_TYPE asGameStateData;
};

class PluginDataRequest
{
  public:
	PluginDataRequestType type;
	PluginRespondDataCallable func;
	PluginDataRequest(PluginDataRequestType type, PluginRespondDataCallable func) : type(type), func(func) {}
};

class PluginCommunicationHandler
{
  public:

	void RunFrame();
	void PushRequest(PluginDataRequestType type, PluginRespondDataCallable func);

  public:
	std::queue<PluginDataRequest> request_queue;
	std::mutex request_mutex;

	bool GenerateServerData(PluginServerData* data);
	bool GenerateGameStateData(PluginGameStateData* data);
};

void init_plugincommunicationhandler();

extern PluginCommunicationHandler* g_pPluginCommunicationhandler;
