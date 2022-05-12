#include "pch.h"
#include "hooks.h"
#include "scriptmodmenu.h"
#include "modmanager.h"
#include "squirrel.h"

// array<string> function NSGetModNames()
SQRESULT SQ_GetModNames(void* sqvm)
{
	g_pClientSquirrel->sq_newarray(sqvm, 0);

	for (Mod& mod : g_pModManager->m_loadedMods)
	{
		g_pClientSquirrel->sq_pushstring(sqvm, mod.Name.c_str(), -1);
		g_pClientSquirrel->sq_arrayappend(sqvm, -2);
	}

	return SQRESULT_NOTNULL;
}

// bool function NSIsModEnabled(string modName)
SQRESULT SQ_IsModEnabled(void* sqvm)
{
	const SQChar* modName = g_pClientSquirrel->sq_getstring(sqvm, 1);

	// manual lookup, not super performant but eh not a big deal
	for (Mod& mod : g_pModManager->m_loadedMods)
	{
		if (!mod.Name.compare(modName))
		{
			g_pClientSquirrel->sq_pushbool(sqvm, mod.Enabled);
			return SQRESULT_NOTNULL;
		}
	}

	return SQRESULT_NULL;
}

// void function NSSetModEnabled(string modName, bool enabled)
SQRESULT SQ_SetModEnabled(void* sqvm)
{
	const SQChar* modName = g_pClientSquirrel->sq_getstring(sqvm, 1);
	const SQBool enabled = g_pClientSquirrel->sq_getbool(sqvm, 2);

	// manual lookup, not super performant but eh not a big deal
	for (Mod& mod : g_pModManager->m_loadedMods)
	{
		if (!mod.Name.compare(modName))
		{
			mod.Enabled = enabled;
			return SQRESULT_NULL;
		}
	}

	return SQRESULT_NULL;
}

// string function NSGetModDescriptionByModName(string modName)
SQRESULT SQ_GetModDescription(void* sqvm)
{
	const SQChar* modName = g_pClientSquirrel->sq_getstring(sqvm, 1);

	// manual lookup, not super performant but eh not a big deal
	for (Mod& mod : g_pModManager->m_loadedMods)
	{
		if (!mod.Name.compare(modName))
		{
			g_pClientSquirrel->sq_pushstring(sqvm, mod.Description.c_str(), -1);
			return SQRESULT_NOTNULL;
		}
	}

	return SQRESULT_NULL;
}

// string function NSGetModVersionByModName(string modName)
SQRESULT SQ_GetModVersion(void* sqvm)
{
	const SQChar* modName = g_pClientSquirrel->sq_getstring(sqvm, 1);

	// manual lookup, not super performant but eh not a big deal
	for (Mod& mod : g_pModManager->m_loadedMods)
	{
		if (!mod.Name.compare(modName))
		{
			g_pClientSquirrel->sq_pushstring(sqvm, mod.Version.c_str(), -1);
			return SQRESULT_NOTNULL;
		}
	}

	return SQRESULT_NULL;
}

// string function NSGetModDownloadLinkByModName(string modName)
SQRESULT SQ_GetModDownloadLink(void* sqvm)
{
	const SQChar* modName = g_pClientSquirrel->sq_getstring(sqvm, 1);

	// manual lookup, not super performant but eh not a big deal
	for (Mod& mod : g_pModManager->m_loadedMods)
	{
		if (!mod.Name.compare(modName))
		{
			g_pClientSquirrel->sq_pushstring(sqvm, mod.DownloadLink.c_str(), -1);
			return SQRESULT_NOTNULL;
		}
	}

	return SQRESULT_NULL;
}

// int function NSGetModLoadPriority(string modName)
SQRESULT SQ_GetModLoadPriority(void* sqvm)
{
	const SQChar* modName = g_pClientSquirrel->sq_getstring(sqvm, 1);

	// manual lookup, not super performant but eh not a big deal
	for (Mod& mod : g_pModManager->m_loadedMods)
	{
		if (!mod.Name.compare(modName))
		{
			g_pClientSquirrel->sq_pushinteger(sqvm, mod.LoadPriority);
			return SQRESULT_NOTNULL;
		}
	}

	return SQRESULT_NULL;
}

// bool function NSIsModRequiredOnClient(string modName)
SQRESULT SQ_IsModRequiredOnClient(void* sqvm)
{
	const SQChar* modName = g_pClientSquirrel->sq_getstring(sqvm, 1);

	// manual lookup, not super performant but eh not a big deal
	for (Mod& mod : g_pModManager->m_loadedMods)
	{
		if (!mod.Name.compare(modName))
		{
			g_pClientSquirrel->sq_pushbool(sqvm, mod.RequiredOnClient);
			return SQRESULT_NOTNULL;
		}
	}

	return SQRESULT_NULL;
}

// array<string> function NSGetModConvarsByModName(string modName)
SQRESULT SQ_GetModConvars(void* sqvm)
{
	const SQChar* modName = g_pClientSquirrel->sq_getstring(sqvm, 1);
	g_pClientSquirrel->sq_newarray(sqvm, 0);

	// manual lookup, not super performant but eh not a big deal
	for (Mod& mod : g_pModManager->m_loadedMods)
	{
		if (!mod.Name.compare(modName))
		{
			for (ModConVar* cvar : mod.ConVars)
			{
				g_pClientSquirrel->sq_pushstring(sqvm, cvar->Name.c_str(), -1);
				g_pClientSquirrel->sq_arrayappend(sqvm, -2);
			}

			return SQRESULT_NOTNULL;
		}
	}

	return SQRESULT_NOTNULL; // return empty array
}

// void function NSReloadMods()
SQRESULT SQ_ReloadMods(void* sqvm)
{
	g_pModManager->LoadMods();
	return SQRESULT_NULL;
}

ON_DLL_LOAD_CLIENT_RELIESON("client.dll", ScriptModMenu, ClientSquirrel, [](HMODULE baseAddress)
{
	g_pUISquirrel->AddFuncRegistration("array<string>", "NSGetModNames", "", "Returns the names of all loaded mods", SQ_GetModNames);
	g_pUISquirrel->AddFuncRegistration(
		"bool", "NSIsModEnabled", "string modName", "Returns whether a given mod is enabled", SQ_IsModEnabled);
	g_pUISquirrel->AddFuncRegistration(
		"void", "NSSetModEnabled", "string modName, bool enabled", "Sets whether a given mod is enabled", SQ_SetModEnabled);
	g_pUISquirrel->AddFuncRegistration(
		"string", "NSGetModDescriptionByModName", "string modName", "Returns a given mod's description", SQ_GetModDescription);
	g_pUISquirrel->AddFuncRegistration(
		"string", "NSGetModVersionByModName", "string modName", "Returns a given mod's version", SQ_GetModVersion);
	g_pUISquirrel->AddFuncRegistration(
		"string", "NSGetModDownloadLinkByModName", "string modName", "Returns a given mod's download link", SQ_GetModDownloadLink);
	g_pUISquirrel->AddFuncRegistration(
		"bool",
		"NSIsModRequiredOnClient",
		"string modName",
		"Returns whether a given mod is required on connecting clients",
		SQ_IsModRequiredOnClient);
	g_pUISquirrel->AddFuncRegistration(
		"int", "NSGetModLoadPriority", "string modName", "Returns a given mod's load priority", SQ_GetModLoadPriority);
	g_pUISquirrel->AddFuncRegistration(
		"array<string>", "NSGetModConvarsByModName", "string modName", "Returns the names of all a given mod's cvars", SQ_GetModConvars);

	g_pUISquirrel->AddFuncRegistration("void", "NSReloadMods", "", "Reloads mods", SQ_ReloadMods);
})