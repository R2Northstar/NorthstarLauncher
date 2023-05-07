#include "mods/modmanager.h"
#include "squirrel/squirrel.h"

template <ScriptContext context> void PushModStruct(HSquirrelVM* sqvm, const Mod& mod, SQInteger index)
{
	g_pSquirrel<context>->pushnewstructinstance(sqvm, 10);

	// index
	g_pSquirrel<context>->pushinteger(sqvm, index);
	g_pSquirrel<context>->sealstructslot(sqvm, 0);

	// name
	g_pSquirrel<context>->pushstring(sqvm, mod.Name.c_str(), -1);
	g_pSquirrel<context>->sealstructslot(sqvm, 1);

	// description
	g_pSquirrel<context>->pushstring(sqvm, mod.Description.c_str(), -1);
	g_pSquirrel<context>->sealstructslot(sqvm, 2);

	// version
	g_pSquirrel<context>->pushstring(sqvm, mod.Version.c_str(), -1);
	g_pSquirrel<context>->sealstructslot(sqvm, 3);

	// link
	g_pSquirrel<context>->pushstring(sqvm, mod.DownloadLink.c_str(), -1);
	g_pSquirrel<context>->sealstructslot(sqvm, 4);

	// installLocation
	g_pSquirrel<context>->pushstring(sqvm, mod.m_ModDirectory.generic_string().c_str());
	g_pSquirrel<context>->sealstructslot(sqvm, 5);

	// loadPriority
	g_pSquirrel<context>->pushinteger(sqvm, mod.LoadPriority);
	g_pSquirrel<context>->sealstructslot(sqvm, 6);

	// requiredOnClient
	g_pSquirrel<context>->pushbool(sqvm, mod.RequiredOnClient);
	g_pSquirrel<context>->sealstructslot(sqvm, 7);

	// enabled
	g_pSquirrel<context>->pushbool(sqvm, mod.m_bEnabled);
	g_pSquirrel<context>->sealstructslot(sqvm, 8);

	// conVars
	g_pSquirrel<context>->newarray(sqvm, 0);
	for (ModConVar* conVar : mod.ConVars)
	{
		g_pSquirrel<context>->pushstring(sqvm, conVar->Name.c_str());
		g_pSquirrel<context>->arrayappend(sqvm, -2);
	}
	g_pSquirrel<context>->sealstructslot(sqvm, 9);

	g_pSquirrel<context>->arrayappend(sqvm, -2);
}

ADD_SQFUNC("void", NSSetModEnabled, "int index, bool enabled", "", ScriptContext::SERVER | ScriptContext::CLIENT | ScriptContext::UI)
{
	const int index = g_pSquirrel<context>->getinteger(sqvm, 1);
	const SQBool enabled = g_pSquirrel<context>->getbool(sqvm, 2);

	if (index < 0 || index >= g_pModManager->m_LoadedMods.size())
	{
		spdlog::warn("Tried disabling mod at {} but only {} mods are loaded", index, g_pModManager->m_LoadedMods.size());
		return SQRESULT_ERROR;
	}

	g_pModManager->m_LoadedMods[index].m_bEnabled = enabled;

	return SQRESULT_NULL;
}

ADD_SQFUNC("void", NSReloadMods, "", "", ScriptContext::SERVER | ScriptContext::CLIENT | ScriptContext::UI)
{
	g_pModManager->LoadMods();
	return SQRESULT_NULL;
}

ADD_SQFUNC(
	"array<Mod>",
	NSGetInvalidMods,
	"",
	"Get all mods that are installed in a subfolder",
	ScriptContext::SERVER | ScriptContext::CLIENT | ScriptContext::UI)
{
	g_pSquirrel<context>->newarray(sqvm, 0);

	for (auto& invalidMod : g_pModManager->m_invalidMods)
	{
		const Mod& mod = *invalidMod.get();
		PushModStruct<context>(sqvm, mod, -1);
	}

	return SQRESULT_NOTNULL;
}

ADD_SQFUNC("array<Mod>", NSGetMods, "", "Get all installed mods", ScriptContext::SERVER | ScriptContext::CLIENT | ScriptContext::UI)
{
	g_pSquirrel<context>->newarray(sqvm, 0);

	for (SQInteger i = 0; i < g_pModManager->m_LoadedMods.size(); i++)
	{
		const Mod& mod = g_pModManager->m_LoadedMods[i];

		PushModStruct<context>(sqvm, mod, i);
	}

	return SQRESULT_NOTNULL;
}
