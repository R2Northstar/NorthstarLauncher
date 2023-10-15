#include "squirrel/squirrel.h"
#include "masterserver/masterserver.h"
#include "server/auth/serverauthentication.h"
#include "dedicated/dedicated.h"
#include "client/r2client.h"
#include "server/r2server.h"

#include <filesystem>

ADD_SQFUNC("void", NSEarlyWritePlayerPersistenceForLeave, "entity player", "", ScriptContext::SERVER)
{
	const R2::CBasePlayer* pPlayer = g_pSquirrel<context>->template getentity<R2::CBasePlayer>(sqvm, 1);
	if (!pPlayer)
	{
		spdlog::warn("NSEarlyWritePlayerPersistenceForLeave got null player");

		g_pSquirrel<context>->pushbool(sqvm, false);
		return SQRESULT_NOTNULL;
	}

	R2::CBaseClient* pClient = &R2::g_pClientArray[pPlayer->m_nPlayerIndex - 1];
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
	const R2::CBasePlayer* pPlayer = g_pSquirrel<ScriptContext::SERVER>->template getentity<R2::CBasePlayer>(sqvm, 1);
	if (!pPlayer)
	{
		spdlog::warn("NSIsPlayerLocalPlayer got null player");

		g_pSquirrel<context>->pushbool(sqvm, false);
		return SQRESULT_NOTNULL;
	}

	R2::CBaseClient* pClient = &R2::g_pClientArray[pPlayer->m_nPlayerIndex - 1];
	g_pSquirrel<context>->pushbool(sqvm, !strcmp(R2::g_pLocalPlayerUserID, pClient->m_UID));
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
	const R2::CBasePlayer* pPlayer = g_pSquirrel<context>->template getentity<R2::CBasePlayer>(sqvm, 1);
	const char* reason = g_pSquirrel<context>->getstring(sqvm, 2);

	if (!pPlayer)
	{
		spdlog::warn("Attempted to call NSDisconnectPlayer() with null player.");

		g_pSquirrel<context>->pushbool(sqvm, false);
		return SQRESULT_NOTNULL;
	}

	// Shouldn't happen but I like sanity checks.
	R2::CBaseClient* pClient = &R2::g_pClientArray[pPlayer->m_nPlayerIndex - 1];
	if (!pClient)
	{
		spdlog::warn("NSDisconnectPlayer(): player entity has null CBaseClient!");

		g_pSquirrel<context>->pushbool(sqvm, false);
		return SQRESULT_NOTNULL;
	}

	if (reason)
	{
		R2::CBaseClient__Disconnect(pClient, 1, reason);
	}
	else
	{
		R2::CBaseClient__Disconnect(pClient, 1, "Disconnected by the server.");
	}

	g_pSquirrel<context>->pushbool(sqvm, true);
	return SQRESULT_NOTNULL;
}

ADD_SQFUNC("array<StatusEffectData>", GetPlayerStatusEffects, "entity player", "", ScriptContext::CLIENT | ScriptContext::SERVER)
{
	const R2::CBasePlayer* pPlayer = g_pSquirrel<context>->template getentity<R2::CBasePlayer>(sqvm, 1);

	g_pSquirrel<context>->newarray(sqvm, 0);
	for (int i = 0; i < 10; i++)
	{
		R2::StatusEffectTimedData effect;
		if (context == ScriptContext::CLIENT)
			effect = pPlayer->m_clientStatusEffectsTimedPlayerNV[i];
		else
			effect = pPlayer->m_statusEffectsTimedPlayerNV[i];

		float intensity = ((effect.seComboVars >> 8) % (1 << 8)) / 255.0;
		if (intensity == 0)
			continue;

		g_pSquirrel<context>->pushnewstructinstance(sqvm, 6);

		g_pSquirrel<context>->pushbool(sqvm, false); // isEndless
		g_pSquirrel<context>->sealstructslot(sqvm, 0);

		g_pSquirrel<context>->pushinteger(sqvm, (effect.seComboVars >> 26)); // id
		g_pSquirrel<context>->sealstructslot(sqvm, 1);

		g_pSquirrel<context>->pushinteger(sqvm, (effect.seComboVars >> 16) % (1 << 10)); // instanceId
		g_pSquirrel<context>->sealstructslot(sqvm, 2);

		g_pSquirrel<context>->pushfloat(sqvm, intensity); // intensity
		g_pSquirrel<context>->sealstructslot(sqvm, 3);

		g_pSquirrel<context>->pushfloat(sqvm, effect.seTimeEnd); // endTime
		g_pSquirrel<context>->sealstructslot(sqvm, 4);

		g_pSquirrel<context>->pushfloat(sqvm, effect.seFadeOutTime); // fadeOutTime
		g_pSquirrel<context>->sealstructslot(sqvm, 5);

		g_pSquirrel<context>->arrayappend(sqvm, -2);

		// spdlog::info("Player status effect {} {}, end time {}", effect.seComboVars, effect.seComboVars2, effect.seTimeEnd);
	}

	for (int i = 0; i < 10; i++)
	{
		R2::StatusEffectEndlessData effect;
		if (context == ScriptContext::CLIENT)
			effect = pPlayer->m_clientStatusEffectsEndlessPlayerNV[i];
		else
			effect = pPlayer->m_statusEffectsEndlessPlayerNV[i];

		float intensity = ((effect.seComboVars >> 8) % (1 << 8)) / 255.0;
		if (intensity == 0)
			continue;

		g_pSquirrel<context>->pushnewstructinstance(sqvm, 6);

		g_pSquirrel<context>->pushbool(sqvm, true); // isEndless
		g_pSquirrel<context>->sealstructslot(sqvm, 0);

		g_pSquirrel<context>->pushinteger(sqvm, (effect.seComboVars >> 26)); // id
		g_pSquirrel<context>->sealstructslot(sqvm, 1);

		g_pSquirrel<context>->pushinteger(sqvm, (effect.seComboVars >> 16) % (1 << 10)); // instanceId
		g_pSquirrel<context>->sealstructslot(sqvm, 2);

		g_pSquirrel<context>->pushfloat(sqvm, intensity); // intensity
		g_pSquirrel<context>->sealstructslot(sqvm, 3);

		g_pSquirrel<context>->pushfloat(sqvm, -1); // endTime
		g_pSquirrel<context>->sealstructslot(sqvm, 4);

		g_pSquirrel<context>->pushfloat(sqvm, -1); // fadeOutTime
		g_pSquirrel<context>->sealstructslot(sqvm, 5);

		g_pSquirrel<context>->arrayappend(sqvm, -2);
	}

	return SQRESULT_NOTNULL;
}
