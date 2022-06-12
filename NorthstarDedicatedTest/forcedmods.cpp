#include "pch.h"
#include "forcedmods.h"

#include "squirrel.h"
#include <iostream>
#include <optional>
#include <utility>

std::vector<std::string> enabledMods;
std::vector<std::string> disabledMods;

// array<string> function NSGetForcedEnabledMods()
SQRESULT SQ_GetForcedEnabledMods(void* sqvm)
{
	ClientSq_newarray(sqvm, 0);

	for (std::string mod : enabledMods)
	{
		ClientSq_pushstring(sqvm, mod.c_str(), -1);
		ClientSq_arrayappend(sqvm, -2);
	}

	return SQRESULT_NOTNULL;
}

// array<string> function NSGetForcedDisableddMods()
SQRESULT SQ_GetForcedDisabledMods(void* sqvm)
{
	ClientSq_newarray(sqvm, 0);

	for (std::string mod : disabledMods)
	{
		ClientSq_pushstring(sqvm, mod.c_str(), -1);
		ClientSq_arrayappend(sqvm, -2);
	}

	return SQRESULT_NOTNULL;
}

// void NSClearForcedEnabledMods()
SQRESULT SQ_ClearForcedEnabledMods(void* sqvm)
{
	enabledMods.clear();
	return SQRESULT_NULL;
}

// void NSClearForcedDisabledMods()
SQRESULT SQ_ClearForcedDisabledMods(void* sqvm)
{
	disabledMods.clear();
	return SQRESULT_NULL;
}

// void NSAddForcedEnabledMod( string mod )
SQRESULT SQ_AddForcedEnabledMod(void* sqvm)
{
	enabledMods.push_back(ClientSq_getstring(sqvm, 1));
	return SQRESULT_NULL;
}

// void NSAddForcedDisabledMod( string mod )
SQRESULT SQ_AddForcedDisabledMod(void* sqvm)
{
	disabledMods.push_back(ClientSq_getstring(sqvm, 1));
	return SQRESULT_NULL;
}

void InitialiseExposedForcedMods(HMODULE baseAddress)
{
	g_UISquirrelManager->AddFuncRegistration("array<string>", "NSGetForcedEnabledMods", "", "", SQ_GetForcedEnabledMods);
	g_UISquirrelManager->AddFuncRegistration("array<string>", "NSGetForcedDisabledMods", "", "", SQ_GetForcedDisabledMods);
	g_UISquirrelManager->AddFuncRegistration("void", "NSClearForcedEnabledMods", "", "", SQ_ClearForcedEnabledMods);
	g_UISquirrelManager->AddFuncRegistration("void", "NSClearForcedDisabledMods", "", "", SQ_ClearForcedDisabledMods);
	g_UISquirrelManager->AddFuncRegistration("void", "NSAddForcedEnabledMod", "string mod", "", SQ_AddForcedEnabledMod);
	g_UISquirrelManager->AddFuncRegistration("void", "NSAddForcedDisabledMod", "string mod", "", SQ_AddForcedDisabledMod);
}
