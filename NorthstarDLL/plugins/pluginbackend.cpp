#include "pluginbackend.h"
#include "plugin_abi.h"
#include "server/serverpresence.h"
#include "masterserver/masterserver.h"
#include "squirrel/squirrel.h"
#include "plugins.h"

#include "core/convar/concommand.h"

#include <filesystem>

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

void InformPluginsDLLLoad(const char* dllPath, void* address)
{
	std::string dllName = fs::path(dllPath).filename().string();

	void* data = NULL;
	if (strncmp(dllName.c_str(), "engine.dll", 10) == 0)
		data = &g_pPluginCommunicationhandler->m_sEngineData;

	g_pPluginManager->InformDLLLoad(dllName.c_str(), data, address);
}
