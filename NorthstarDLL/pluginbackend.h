#pragma once
#include "pch.h"
#include "plugin_abi.h"

#include <queue>
#include <mutex>

enum PluginDataRequestType
{
	END = 0,
};

union PluginRespondDataCallable
{
	// Empty for now
	void* UNUSED;
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

	bool PushPresenceData(PluginGameStatePresence* data);
};

void init_plugincommunicationhandler();

extern PluginCommunicationHandler* g_pPluginCommunicationhandler;
