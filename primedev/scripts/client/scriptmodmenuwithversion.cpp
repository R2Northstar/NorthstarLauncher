#include "mods/modmanager.h"
#include "squirrel/squirrel.h"

ADD_SQFUNC("array<RequiredModInfo>", NSGetModNamesAndVersions, "", "", ScriptContext::SERVER | ScriptContext::CLIENT | ScriptContext::UI)
{
	g_pSquirrel<context>->newarray(sqvm, 0);

	for (Mod& mod : g_pModManager->m_LoadedMods)
	{
		g_pSquirrel<context>->pushnewstructinstance(sqvm, 2);

		// name
		g_pSquirrel<context>->pushstring(sqvm, mod.Name.c_str(), -1);
		g_pSquirrel<context>->sealstructslot(sqvm, 0);

		// version
		g_pSquirrel<context>->pushstring(sqvm, mod.Version.c_str(), -1);
		g_pSquirrel<context>->sealstructslot(sqvm, 1);

		g_pSquirrel<context>->arrayappend(sqvm, -2);
	}

	return SQRESULT_NOTNULL;
}

ADD_SQFUNC("void", NSSetModEnabledWithVersion, "string modName, string modVersion, bool enabled", "", ScriptContext::UI)
{
	const SQChar* modName = g_pSquirrel<context>->getstring(sqvm, 1);
	const SQChar* modVersion = g_pSquirrel<context>->getstring(sqvm, 1);
	const SQBool enabled = g_pSquirrel<context>->getbool(sqvm, 3);

	// manual lookup, not super performant but eh not a big deal
	for (Mod& mod : g_pModManager->m_LoadedMods)
	{
		if (mod.Name.compare(modName) && mod.Version.compare(modVersion))
		{
			mod.m_bEnabled = enabled;
			return SQRESULT_NULL;
		}
	}

	return SQRESULT_NULL;
}

ADD_SQFUNC(
	"bool",
	NSIsModRequiredOnClientWithVersion,
	"string modName, string modVersion",
	"",
	ScriptContext::SERVER | ScriptContext::CLIENT | ScriptContext::UI)
{
	const SQChar* modName = g_pSquirrel<context>->getstring(sqvm, 1);
	const SQChar* modVersion = g_pSquirrel<context>->getstring(sqvm, 2);

	// manual lookup, not super performant but eh not a big deal
	for (Mod& mod : g_pModManager->m_LoadedMods)
	{
		if (!mod.Name.compare(modName) && !mod.Version.compare(modVersion))
		{
			g_pSquirrel<context>->pushbool(sqvm, mod.RequiredOnClient);
			return SQRESULT_NOTNULL;
		}
	}

	return SQRESULT_NULL;
}
