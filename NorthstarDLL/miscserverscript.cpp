#include "pch.h"
#include "squirrel.h"
#include "masterserver.h"
#include "serverauthentication.h"
#include "r2client.h"
#include "r2server.h"

#include <filesystem>

// void function NSEarlyWritePlayerIndexPersistenceForLeave( int playerIndex )
SQRESULT SQ_EarlyWritePlayerIndexPersistenceForLeave(HSquirrelVM* sqvm)
{
	int playerIndex = g_pServerSquirrel->getinteger(sqvm, 1);
	R2::CBaseClient* player = &R2::g_pClientArray[playerIndex];

	if (!g_pServerAuthentication->m_PlayerAuthenticationData.count(player))
	{
		g_pServerSquirrel->raiseerror(sqvm, fmt::format("Invalid playerindex {}", playerIndex).c_str());
		return SQRESULT_ERROR;
	}

	g_pServerAuthentication->m_PlayerAuthenticationData[player].needPersistenceWriteOnLeave = false;
	g_pServerAuthentication->WritePersistentData(player);
	return SQRESULT_NULL;
}

// bool function NSIsWritingPlayerPersistence()
SQRESULT SQ_IsWritingPlayerPersistence(HSquirrelVM* sqvm)
{
	g_pServerSquirrel->pushbool(sqvm, g_pMasterServerManager->m_bSavingPersistentData);
	return SQRESULT_NOTNULL;
}

// bool function NSIsPlayerIndexLocalPlayer( int playerIndex )
SQRESULT SQ_IsPlayerIndexLocalPlayer(HSquirrelVM* sqvm)
{
	int playerIndex = g_pServerSquirrel->getinteger(sqvm, 1);
	R2::CBaseClient* player = &R2::g_pClientArray[playerIndex];
	if (!g_pServerAuthentication->m_PlayerAuthenticationData.count(player))
	{
		g_pServerSquirrel->raiseerror(sqvm, fmt::format("Invalid playerindex {}", playerIndex).c_str());
		return SQRESULT_ERROR;
	}

	g_pServerSquirrel->pushbool(sqvm, !strcmp(R2::g_pLocalPlayerUserID, player->m_UID));
	return SQRESULT_NOTNULL;
}

ON_DLL_LOAD_RELIESON("server.dll", MiscServerScriptCommands, ServerSquirrel, (CModule module))
{
	g_pServerSquirrel->AddFuncRegistration(
		"void", "NSEarlyWritePlayerIndexPersistenceForLeave", "int playerIndex", "", SQ_EarlyWritePlayerIndexPersistenceForLeave);
	g_pServerSquirrel->AddFuncRegistration("bool", "NSIsWritingPlayerPersistence", "", "", SQ_IsWritingPlayerPersistence);
	g_pServerSquirrel->AddFuncRegistration("bool", "NSIsPlayerIndexLocalPlayer", "int playerIndex", "", SQ_IsPlayerIndexLocalPlayer);
}