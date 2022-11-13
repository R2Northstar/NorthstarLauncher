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

// asset function GetUserInfoKVAsset_Internal( entity player, string key, asset defaultValue = $"" )
SQRESULT SQ_GetUserInfoKVAsset_Internal(HSquirrelVM* sqvm)
{
	const R2::CBasePlayer* pPlayer = g_pSquirrel<ScriptContext::SERVER>->getentity<R2::CBasePlayer>(sqvm, 1);
	if (!pPlayer)
	{
		g_pSquirrel<ScriptContext::SERVER>->raiseerror(sqvm, "player is null");
		return SQRESULT_ERROR;
	}

	const char* pKey = g_pSquirrel<ScriptContext::SERVER>->getstring(sqvm, 2);
	const char* pDefaultValue;
	g_pSquirrel<ScriptContext::SERVER>->getasset(sqvm, 3, &pDefaultValue);

	const char* pResult = R2::g_pClientArray[pPlayer->m_nPlayerIndex - 1].m_ConVars->GetString(pKey, pDefaultValue);
	g_pSquirrel<ScriptContext::SERVER>->pushasset(sqvm, pResult);
	return SQRESULT_NOTNULL;
}

// int function GetUserInfoKVInt_Internal( entity player, string key, int defaultValue = 0 )
SQRESULT SQ_GetUserInfoKVInt_Internal(HSquirrelVM* sqvm)
{
	const R2::CBasePlayer* pPlayer = g_pSquirrel<ScriptContext::SERVER>->getentity<R2::CBasePlayer>(sqvm, 1);
	if (!pPlayer)
	{
		g_pSquirrel<ScriptContext::SERVER>->raiseerror(sqvm, "player is null");
		return SQRESULT_ERROR;
	}

	const char* pKey = g_pSquirrel<ScriptContext::SERVER>->getstring(sqvm, 2);
	const int iDefaultValue = g_pSquirrel<ScriptContext::SERVER>->getinteger(sqvm, 3);

	const int iResult = R2::g_pClientArray[pPlayer->m_nPlayerIndex - 1].m_ConVars->GetInt(pKey, iDefaultValue);
	g_pSquirrel<ScriptContext::SERVER>->pushinteger(sqvm, iResult);
	return SQRESULT_NOTNULL;
}

// float function GetUserInfoKVFloat_Internal( entity player, string key, float defaultValue = 0 )
SQRESULT SQ_GetUserInfoKVFloat_Internal(HSquirrelVM* sqvm)
{
	const R2::CBasePlayer* pPlayer = g_pSquirrel<ScriptContext::SERVER>->getentity<R2::CBasePlayer>(sqvm, 1);
	if (!pPlayer)
	{
		g_pSquirrel<ScriptContext::SERVER>->raiseerror(sqvm, "player is null");
		return SQRESULT_ERROR;
	}

	const char* pKey = g_pSquirrel<ScriptContext::SERVER>->getstring(sqvm, 2);
	const float flDefaultValue = g_pSquirrel<ScriptContext::SERVER>->getfloat(sqvm, 3);

	const float flResult = R2::g_pClientArray[pPlayer->m_nPlayerIndex - 1].m_ConVars->GetFloat(pKey, flDefaultValue);
	g_pSquirrel<ScriptContext::SERVER>->pushfloat(sqvm, flResult);
	return SQRESULT_NOTNULL;
}

// bool function GetUserInfoKVBool_Internal( entity player, string key, bool defaultValue = false )
SQRESULT SQ_GetUserInfoKVBool_Internal(HSquirrelVM* sqvm)
{
	const R2::CBasePlayer* pPlayer = g_pSquirrel<ScriptContext::SERVER>->getentity<R2::CBasePlayer>(sqvm, 1);
	if (!pPlayer)
	{
		g_pSquirrel<ScriptContext::SERVER>->raiseerror(sqvm, "player is null");
		return SQRESULT_ERROR;
	}

	const char* pKey = g_pSquirrel<ScriptContext::SERVER>->getstring(sqvm, 2);
	const bool bDefaultValue = g_pSquirrel<ScriptContext::SERVER>->getbool(sqvm, 3);

	const bool bResult = R2::g_pClientArray[pPlayer->m_nPlayerIndex - 1].m_ConVars->GetInt(pKey, bDefaultValue);
	g_pSquirrel<ScriptContext::SERVER>->pushbool(sqvm, bResult);
	return SQRESULT_NOTNULL;
}

ON_DLL_LOAD_RELIESON("server.dll", ScriptUserInfoKVs, ServerSquirrel, (CModule module))
{
	g_pSquirrel<ScriptContext::SERVER>->AddFuncRegistration(
		"string", "GetUserInfoKVString_Internal", "entity player, string key, string defaultValue =	\"\"", "", SQ_GetUserInfoKVString_Internal);
	g_pSquirrel<ScriptContext::SERVER>->AddFuncRegistration(
		"asset", "GetUserInfoKVAsset_Internal", "entity player, string key, asset defaultValue = $\"\"", "", SQ_GetUserInfoKVAsset_Internal);
	g_pSquirrel<ScriptContext::SERVER>->AddFuncRegistration(
		"int", "GetUserInfoKVInt_Internal", "entity player, string key, int defaultValue = 0", "", SQ_GetUserInfoKVInt_Internal);
	g_pSquirrel<ScriptContext::SERVER>->AddFuncRegistration(
		"float", "GetUserInfoKVFloat_Internal", "entity player, string key, float defaultValue = 0", "", SQ_GetUserInfoKVFloat_Internal);
	g_pSquirrel<ScriptContext::SERVER>->AddFuncRegistration(
		"bool", "GetUserInfoKVBool_Internal", "entity player, string key, bool defaultValue = 0", "", SQ_GetUserInfoKVBool_Internal);
}
