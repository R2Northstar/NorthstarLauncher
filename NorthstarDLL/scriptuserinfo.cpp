#include "pch.h"
#include "squirrel.h"
#include "r2engine.h"
#include "r2server.h"

// TODO: in theory we could support other types here, but effort atm

// string function GetUserInfoKVString_Internal( entity player, string key, string defaultValue = "" )
SQRESULT SQ_GetUserInfoKVString_Internal(HSquirrelVM* sqvm)
{
	const R2::CBasePlayer* pPlayer = g_pSquirrel<ScriptContext::SERVER>->getentity<R2::CBasePlayer>(sqvm, 1);
	if (!pPlayer)
	{
		g_pSquirrel<ScriptContext::SERVER>->raiseerror(sqvm, "player is null");
		return SQRESULT_ERROR;
	}

	const char* pKey = g_pSquirrel<ScriptContext::SERVER>->getstring(sqvm, 2);
	const char* pDefaultValue = g_pSquirrel<ScriptContext::SERVER>->getstring(sqvm, 3);

	const char* pResult = R2::g_pClientArray[pPlayer->m_nPlayerIndex - 1].m_ConVars->GetString(pKey, pDefaultValue);
	g_pSquirrel<ScriptContext::SERVER>->pushstring(sqvm, pResult);
	return SQRESULT_NOTNULL;
}

// string ornull function GetUserInfoKVStringOrNull_Internal( entity player, string key )
SQRESULT SQ_GetUserInfoKVStringOrNull_Internal(HSquirrelVM* sqvm)
{
	const R2::CBasePlayer* pPlayer = g_pSquirrel<ScriptContext::SERVER>->getentity<R2::CBasePlayer>(sqvm, 1);
	if (!pPlayer)
	{
		g_pSquirrel<ScriptContext::SERVER>->raiseerror(sqvm, "player is null");
		return SQRESULT_ERROR;
	}

	const char* pKey = g_pSquirrel<ScriptContext::SERVER>->getstring(sqvm, 2);

	const char* pResult = R2::g_pClientArray[pPlayer->m_nPlayerIndex - 1].m_ConVars->GetString(pKey, nullptr);
	if (!pResult)
		return SQRESULT_NULL;

	g_pSquirrel<ScriptContext::SERVER>->pushstring(sqvm, pResult);
	return SQRESULT_NOTNULL;
}

ON_DLL_LOAD_RELIESON("server.dll", ScriptUserInfoKVs, ServerSquirrel, (CModule module))
{
	g_pSquirrel<ScriptContext::SERVER>->AddFuncRegistration(
		"string", "GetUserInfoKVString_Internal", "entity player, string key, string defaultValue", "", SQ_GetUserInfoKVString_Internal);
	g_pSquirrel<ScriptContext::SERVER>->AddFuncRegistration(
		"string ornull", "GetUserInfoKVStringOrNull_Internal", "entity player, string key", "", SQ_GetUserInfoKVStringOrNull_Internal);
}
