#include "pluginbackend.h"
#include "plugin_abi.h"
#include "server/serverpresence.h"
#include "masterserver/masterserver.h"
#include "squirrel/squirrel.h"
#include "plugins.h"

#include "core/convar/concommand.h"

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

ON_DLL_LOAD_RELIESON("engine.dll", PluginBackendEngine, ConCommand, (CModule module))
{
	g_pPluginManager->InformDLLLoad(
		PluginLoadDLL::ENGINE, &g_pPluginCommunicationhandler->m_sEngineData, reinterpret_cast<void*>(module.m_nAddress));
}

ON_DLL_LOAD_RELIESON("client.dll", PluginBackendClient, ConCommand, (CModule module))
{
	g_pPluginManager->InformDLLLoad(PluginLoadDLL::CLIENT, nullptr, reinterpret_cast<void*>(module.m_nAddress));
}

ON_DLL_LOAD_RELIESON("server.dll", PluginBackendServer, ConCommand, (CModule module))
{
	g_pPluginManager->InformDLLLoad(PluginLoadDLL::SERVER, nullptr, reinterpret_cast<void*>(module.m_nAddress));
}
