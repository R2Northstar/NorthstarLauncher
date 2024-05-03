#pragma once

#include "engine/r2engine.h"

/// Determines if we are in vanilla-compatibility mode.
class VanillaCompatibility
{
public:
	bool GetVanillaCompatibility()
	{
		if (g_pCVar->FindVar("serverfilter")->GetBool() && !g_pCVar->FindVar("ns_is_northstar_server")->GetBool() &&
			!g_pCVar->FindVar("ns_auth_allow_insecure")->GetBool())
		{
			Cbuf_AddText(Cbuf_GetCurrentPlayer(), "disconnect \"Server is outdated or evil\"", cmd_source_t::kCommandSrcCode);

			return false;
		}

		return g_pCVar->FindVar("ns_is_northstar_server")->GetBool() == 0;
	}
};

inline VanillaCompatibility* g_pVanillaCompatibility;
