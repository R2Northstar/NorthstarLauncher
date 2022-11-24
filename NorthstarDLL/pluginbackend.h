#pragma once
#include "pch.h"
#include "plugin_abi.h"
#include "gamepresence.h"

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

	void GeneratePresenceObjects();

  public:
	std::queue<PluginDataRequest> requestQueue;
	std::mutex requestMutex;
};

void init_plugincommunicationhandler();

extern PluginCommunicationHandler* g_pPluginCommunicationhandler;
