#include "client/r2client.h"
#include "core/vanilla.h"
#include "engine/r2engine.h"
#include "masterserver/masterserver.h"
#include "squirrel/squirrel.h"

ADD_SQFUNC("bool", NSIsMasterServerAuthenticated, "", "", ScriptContext::UI)
{
	g_pSquirrel<context>->pushbool(sqvm, g_pMasterServerManager->m_bOriginAuthWithMasterServerDone);
	return SQRESULT_NOTNULL;
}

/*
global struct MasterServerAuthResult
{
	bool success
	string errorCode
	string errorMessage
}
*/

ADD_SQFUNC("void", NSResetToken, "", "", ScriptContext::UI)
{
	g_pCVar->FindVar("serverfilter")->SetValue("");
	return SQRESULT_NULL;
}

ADD_SQFUNC("bool", NSIsVanilla, "", "", ScriptContext::SERVER | ScriptContext::CLIENT | ScriptContext::UI)
{
	g_pSquirrel<context>->pushbool(sqvm, g_pVanillaCompatibility->GetVanillaCompatibility());
	return SQRESULT_NOTNULL;
}

ADD_SQFUNC("MasterServerAuthResult", NSGetMasterServerAuthResult, "", "", ScriptContext::UI)
{
	g_pSquirrel<context>->pushnewstructinstance(sqvm, 3);

	g_pSquirrel<context>->pushbool(sqvm, g_pMasterServerManager->m_bOriginAuthWithMasterServerSuccessful);
	g_pSquirrel<context>->sealstructslot(sqvm, 0);

	g_pSquirrel<context>->pushstring(sqvm, g_pMasterServerManager->m_sOriginAuthWithMasterServerErrorCode.c_str(), -1);
	g_pSquirrel<context>->sealstructslot(sqvm, 1);

	g_pSquirrel<context>->pushstring(sqvm, g_pMasterServerManager->m_sOriginAuthWithMasterServerErrorMessage.c_str(), -1);
	g_pSquirrel<context>->sealstructslot(sqvm, 2);

	return SQRESULT_NOTNULL;
}
