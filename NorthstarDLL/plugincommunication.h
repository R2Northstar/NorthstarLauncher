#pragma once
#include "pch.h"
#include "plugin_abi.h"

#include <queue>

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
	PLUGIN_RESPOND_RPC_DATA_TYPE asRPCData;
};

class PluginDataRequest
{
  public:
	PluginDataRequestType type;
	PluginRespondDataCallable func;
	PluginDataRequest(PluginDataRequestType type, PluginRespondDataCallable func) : type(type), func(func) {}
	~PluginDataRequest()
	{
		spdlog::info("PluginDataRequest destroyed");
	}
};

class PluginCommunicationHandler
{
  public:

	void RunFrame();
	void PushRequest(PluginDataRequestType type, PluginRespondDataCallable func);

  private:
	std::queue<PluginDataRequest> request_queue;

	PluginServerData* GenerateServerData();
	PluginGameStateData* GenerateGameStateData();
};

void init_plugincommunicationhandler();

extern PluginCommunicationHandler* g_pPluginCommunicationhandler;
