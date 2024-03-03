#include "squirrel/squirrel.h"
#include "masterserver/masterserver.h"
#include "server/auth/serverauthentication.h"
#include "dedicated/dedicated.h"
#include "client/r2client.h"
#include "server/r2server.h"

#include <filesystem>

ADD_SQFUNC("void", NSEarlyWritePlayerPersistenceForLeave, "entity player", "", ScriptContext::SERVER)
{
	const CBasePlayer* pPlayer = g_pSquirrel<context>->template getentity<CBasePlayer>(sqvm, 1);
	if (!pPlayer)
	{
		Warning(eLog::NS, "NSEarlyWritePlayerPersistenceForLeave got null player\n");

		g_pSquirrel<context>->pushbool(sqvm, false);
		return SQRESULT_NOTNULL;
	}

	CBaseClient* pClient = &g_pClientArray[pPlayer->m_nPlayerIndex - 1];
	if (g_pServerAuthentication->m_PlayerAuthenticationData.find(pClient) == g_pServerAuthentication->m_PlayerAuthenticationData.end())
	{
		g_pSquirrel<context>->pushbool(sqvm, false);
		return SQRESULT_NOTNULL;
	}

	g_pServerAuthentication->m_PlayerAuthenticationData[pClient].needPersistenceWriteOnLeave = false;
	g_pServerAuthentication->WritePersistentData(pClient);
	return SQRESULT_NULL;
}

ADD_SQFUNC("bool", NSIsWritingPlayerPersistence, "", "", ScriptContext::SERVER)
{
	g_pSquirrel<context>->pushbool(sqvm, g_pMasterServerManager->m_bSavingPersistentData);
	return SQRESULT_NOTNULL;
}

ADD_SQFUNC("bool", NSIsPlayerLocalPlayer, "entity player", "", ScriptContext::SERVER)
{
	const CBasePlayer* pPlayer = g_pSquirrel<ScriptContext::SERVER>->template getentity<CBasePlayer>(sqvm, 1);
	if (!pPlayer)
	{
		Warning(eLog::NS, "NSIsPlayerLocalPlayer got null player\n");

		g_pSquirrel<context>->pushbool(sqvm, false);
		return SQRESULT_NOTNULL;
	}

	CBaseClient* pClient = &g_pClientArray[pPlayer->m_nPlayerIndex - 1];
	g_pSquirrel<context>->pushbool(sqvm, !strcmp(g_pLocalPlayerUserID, pClient->m_UID));
	return SQRESULT_NOTNULL;
}

ADD_SQFUNC("bool", NSIsDedicated, "", "", ScriptContext::SERVER)
{
	g_pSquirrel<context>->pushbool(sqvm, IsDedicatedServer());
	return SQRESULT_NOTNULL;
}

ADD_SQFUNC(
	"bool",
	NSDisconnectPlayer,
	"entity player, string reason",
	"Disconnects the player from the server with the given reason",
	ScriptContext::SERVER)
{
	const CBasePlayer* pPlayer = g_pSquirrel<context>->template getentity<CBasePlayer>(sqvm, 1);
	const char* reason = g_pSquirrel<context>->getstring(sqvm, 2);

	if (!pPlayer)
	{
		Warning(eLog::NS, "Attempted to call NSDisconnectPlayer() with null player.\n");

		g_pSquirrel<context>->pushbool(sqvm, false);
		return SQRESULT_NOTNULL;
	}

	// Shouldn't happen but I like sanity checks.
	CBaseClient* pClient = &g_pClientArray[pPlayer->m_nPlayerIndex - 1];
	if (!pClient)
	{
		Warning(eLog::NS, "NSDisconnectPlayer(): player entity has null CBaseClient!\n");

		g_pSquirrel<context>->pushbool(sqvm, false);
		return SQRESULT_NOTNULL;
	}

	if (reason)
	{
		CBaseClient__Disconnect(pClient, 1, reason);
	}
	else
	{
		CBaseClient__Disconnect(pClient, 1, "Disconnected by the server.");
	}

	g_pSquirrel<context>->pushbool(sqvm, true);
	return SQRESULT_NOTNULL;
}
