#include "squirrel/squirrel.h"
#include "client/r2client.h"
#include "engine/r2engine.h"

// asset function StringToAsset( string assetName )
ADD_SQFUNC(
	"asset",
	StringToAsset,
	"string assetName",
	"converts a given string to an asset",
	ScriptContext::UI | ScriptContext::CLIENT | ScriptContext::SERVER)
{
	g_pSquirrel[context]->pushasset(sqvm, g_pSquirrel[context]->getstring(sqvm, 1), -1);
	return SQRESULT_NOTNULL;
}

// string function NSGetLocalPlayerUID()
ADD_SQFUNC(
	"string", NSGetLocalPlayerUID, "", "Returns the local player's uid.", ScriptContext::UI | ScriptContext::CLIENT | ScriptContext::SERVER)
{
	if (g_pLocalPlayerUserID)
	{
		g_pSquirrel[context]->pushstring(sqvm, g_pLocalPlayerUserID);
		return SQRESULT_NOTNULL;
	}

	return SQRESULT_NULL;
}

// bool function NSIsServerVMValid()
ADD_SQFUNC("bool", NSIsServerVMValid, "", "Returns if server vm is valid.", ScriptContext::UI | ScriptContext::CLIENT)
{
	g_pSquirrel[context]->pushbool(sqvm, g_pSquirrel[ScriptContext::SERVER]->m_pSQVM && g_pSquirrel[ScriptContext::SERVER]->m_pSQVM->sqvm);
	return SQRESULT_NOTNULL;
}
