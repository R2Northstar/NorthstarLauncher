#include "pch.h"
#include "plugincommunication.h"
#include "plugin_abi.h"
#include "serverpresence.h"
#include "masterserver.h"
#include "squirrel.h"

#define EXPORT extern "C" __declspec(dllexport)

AUTOHOOK_INIT()

PluginCommunicationHandler* g_pPluginCommunicationhandler;
PluginGameStateData* l_GameStateData;

static PluginDataRequest request {PluginDataRequestType::GAMESTATE, (PluginRespondDataCallable)0};

void init_plugincommunicationhandler() {
	g_pPluginCommunicationhandler = new PluginCommunicationHandler;
	g_pPluginCommunicationhandler->request_queue = {};
}

void PluginCommunicationHandler::RunFrame() {
	std::lock_guard<std::mutex> lock(request_mutex);
	if (!request_queue.empty())
	{
		request = request_queue.front();
		PluginServerData server {};
		PluginGameStateData* gamestate = new PluginGameStateData();
		// Note: these new/deletes are bad practice, should switch to RAII types instead
		switch (request.type)
		{
			case PluginDataRequestType::SERVER:
				if(GenerateServerData(&server))
					request.func.asServerData(&server);
				else
					request.func.asServerData(nullptr);
				break;
			case PluginDataRequestType::GAMESTATE:
				GenerateGameStateData(gamestate);
				break;
		}
		request_queue.pop();
	}
}

bool PluginCommunicationHandler::GenerateServerData(PluginServerData* data)
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
}

template <ScriptContext context> SQRESULT SQ_PushGameStateData(HSquirrelVM* sqvm)
{

	SQStructInstance* structInst = g_pSquirrel<ScriptContext::CLIENT>->m_pSQVM->sqvm->_stackOfCurrentFunction[1]._VAL.asStructInstance;
	memcpy((void*)l_GameStateData->map, structInst->data[0]._VAL.asString->_val, structInst->data[0]._VAL.asString->length + 1);
	memcpy((void*)l_GameStateData->map_displayname, structInst->data[1]._VAL.asString->_val, structInst->data[1]._VAL.asString->length + 1);
	memcpy((void*)l_GameStateData->playlist, structInst->data[2]._VAL.asString->_val, structInst->data[2]._VAL.asString->length + 1);
	memcpy(
		(void*)l_GameStateData->playlist_displayname,
		structInst->data[3]._VAL.asString->_val,
		structInst->data[3]._VAL.asString->length + 1);

	l_GameStateData->current_players = structInst->data[4]._VAL.asInteger;
	l_GameStateData->max_players = structInst->data[5]._VAL.asInteger;
	l_GameStateData->own_score = structInst->data[6]._VAL.asInteger;
	l_GameStateData->other_highest_score = structInst->data[7]._VAL.asInteger;
	l_GameStateData->max_score = structInst->data[8]._VAL.asInteger;
	l_GameStateData->timestamp_end = structInst->data[9]._VAL.asInteger;

	request.func.asGameStateData(l_GameStateData);

	delete l_GameStateData;

	return SQRESULT_NOTNULL;
}

bool PluginCommunicationHandler::GenerateGameStateData(PluginGameStateData* data)
{
	if (g_pSquirrel<ScriptContext::CLIENT>->m_pSQVM == nullptr || g_pSquirrel<ScriptContext::CLIENT>->m_pSQVM->sqvm == nullptr)
	{
		request.func.asGameStateData(nullptr);
		return false;
	}
	l_GameStateData = data;
	g_pSquirrel<ScriptContext::CLIENT>->call("GenerateGameState");

	return true;
}

void PluginCommunicationHandler::PushRequest(PluginDataRequestType type, PluginRespondDataCallable func)
{
	std::lock_guard<std::mutex> lock(request_mutex);
	request_queue.push(PluginDataRequest {type, func});
}

// TODO: fix this
EXPORT void PLUGIN_REQUESTS_SERVER_DATA(PLUGIN_RESPOND_SERVER_DATA_TYPE func) {
	g_pPluginCommunicationhandler->PushRequest(PluginDataRequestType::SERVER, *(PluginRespondDataCallable*)(&func));
}

EXPORT void PLUGIN_REQUESTS_GAMESTATE_DATA(PLUGIN_RESPOND_GAMESTATE_DATA_TYPE func)
{
	g_pPluginCommunicationhandler->PushRequest(PluginDataRequestType::GAMESTATE, *(PluginRespondDataCallable*)(&func));
}

ON_DLL_LOAD_RELIESON("client.dll", PluginCommunicationSquirrel, ClientSquirrel, (CModule module))
{

	l_GameStateData = new PluginGameStateData {};

	g_pSquirrel<ScriptContext::CLIENT>->AddFuncRegistration(
		"void", "NSPushGameStateData", "var data", "", SQ_PushGameStateData<ScriptContext::CLIENT>);
}
