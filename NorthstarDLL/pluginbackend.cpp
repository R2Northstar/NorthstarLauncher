#include "pch.h"
#include "pluginbackend.h"
#include "plugin_abi.h"
#include "serverpresence.h"
#include "masterserver.h"
#include "squirrel.h"
#include "plugins.h"

#define EXPORT extern "C" __declspec(dllexport)

AUTOHOOK_INIT()

PluginCommunicationHandler* g_pPluginCommunicationhandler;
PluginGameStatePresence* l_GameStatePresenceData;

static PluginDataRequest storedRequest {PluginDataRequestType::END, (PluginRespondDataCallable)nullptr};

void init_plugincommunicationhandler()
{
	g_pPluginCommunicationhandler = new PluginCommunicationHandler;
	g_pPluginCommunicationhandler->request_queue = {};
}

void PluginCommunicationHandler::RunFrame()
{
	std::lock_guard<std::mutex> lock(request_mutex);
	if (!request_queue.empty())
	{
		storedRequest = request_queue.front();
		switch (storedRequest.type)
		{
		default:
			spdlog::error("{} was called with invalid request type '{}'", __FUNCTION__, static_cast<int>(storedRequest.type));
		}
		request_queue.pop();
	}
}

template <ScriptContext context> SQRESULT SQ_PushGameStateData(HSquirrelVM* sqvm)
{

	SQStructInstance* structInst = g_pSquirrel<ScriptContext::CLIENT>->m_pSQVM->sqvm->_stackOfCurrentFunction[1]._VAL.asStructInstance;
	memcpy((void*)l_GameStatePresenceData->map, structInst->data[0]._VAL.asString->_val, structInst->data[0]._VAL.asString->length + 1);
	memcpy((void*)l_GameStatePresenceData->map_displayname, structInst->data[1]._VAL.asString->_val, structInst->data[1]._VAL.asString->length + 1);
	memcpy(
		(void*)l_GameStatePresenceData->playlist, structInst->data[2]._VAL.asString->_val, structInst->data[2]._VAL.asString->length + 1);
	memcpy(
		(void*)l_GameStatePresenceData->playlist_displayname,
		structInst->data[3]._VAL.asString->_val,
		structInst->data[3]._VAL.asString->length + 1);

	l_GameStatePresenceData->current_players = structInst->data[4]._VAL.asInteger;
	l_GameStatePresenceData->max_players = structInst->data[5]._VAL.asInteger;
	l_GameStatePresenceData->own_score = structInst->data[6]._VAL.asInteger;
	l_GameStatePresenceData->other_highest_score = structInst->data[7]._VAL.asInteger;
	l_GameStatePresenceData->max_score = structInst->data[8]._VAL.asInteger;
	l_GameStatePresenceData->timestamp_end = structInst->data[9]._VAL.asInteger;

	g_pPluginManager->PushPresence(l_GameStatePresenceData);
	delete l_GameStatePresenceData;

	return SQRESULT_NOTNULL;
}

bool PluginCommunicationHandler::PushPresenceData(PluginGameStatePresence* data)
{
	if (g_pServerPresence->m_bHasPresence)
	{
		ServerPresence* pr = &g_pServerPresence->m_ServerPresence;
		data->is_local = true;
		memcpy(data->id, pr->m_sServerId.c_str(), pr->m_sServerId.length() + 1);
		memcpy(data->name, pr->m_sServerName.c_str(), pr->m_sServerName.length() + 1);
		memcpy(data->description, pr->m_sServerDesc.c_str(), pr->m_sServerDesc.length() + 1);
		memcpy(data->password, pr->m_Password, sizeof(data->password));
	}
	else
	{
		data->is_local = false;
		if (!g_pMasterServerManager->m_currentServer.has_value())
		{
			memset(data->id, 0, sizeof(data->id));
			memset(data->name, 0, sizeof(data->name));
			memset(data->description, 0, sizeof(data->description));
			memset(data->password, 0, sizeof(data->password));
		}
		else
		{
			RemoteServerInfo& server = g_pMasterServerManager->m_currentServer.value();
			memcpy(data->id, server.id, sizeof(server.id));
			memcpy(data->name, server.name, sizeof(server.name));
			memcpy(data->description, server.description.c_str(), server.description.length() + 1);
			memcpy(
				data->password,
				g_pMasterServerManager->m_sCurrentServerPassword.c_str(),
				g_pMasterServerManager->m_sCurrentServerPassword.length() + 1);
		}
	}
	return true;

	if (g_pSquirrel<ScriptContext::CLIENT> == nullptr || g_pSquirrel<ScriptContext::CLIENT>->m_pSQVM == nullptr || g_pSquirrel<ScriptContext::CLIENT>->m_pSQVM->sqvm == nullptr)
	{
		return false;
	}
	l_GameStatePresenceData = data;
	g_pSquirrel<ScriptContext::CLIENT>->Call("GenerateGameState");

	return true;
}

void PluginCommunicationHandler::PushRequest(PluginDataRequestType type, PluginRespondDataCallable func)
{
	std::lock_guard<std::mutex> lock(request_mutex);
	request_queue.push(PluginDataRequest {type, func});
}

/* Example for later use
EXPORT void PLUGIN_REQUESTS_PRESENCE_DATA(PLUGIN_RESPOND_PRESENCE_DATA_TYPE func)
{
	g_pPluginCommunicationhandler->PushRequest(PluginDataRequestType::PRESENCE, *(PluginRespondDataCallable*)(&func));
}
*/

ON_DLL_LOAD_RELIESON("client.dll", PluginCommunicationSquirrel, ClientSquirrel, (CModule module))
{

	l_GameStatePresenceData = new PluginGameStatePresence {};

	g_pSquirrel<ScriptContext::CLIENT>->AddFuncRegistration(
		"void", "NSPushGameStateData", "var data", "", SQ_PushGameStateData<ScriptContext::CLIENT>);
}
