#pragma once

#include "engine/r2engine.h"
#include "engine/hoststate.h"

/// Determines if we are in vanilla-compatibility mode.
class VanillaCompatibility
{
public:
	bool GetVanillaCompatibility()
	{
		// this usually means something malicious or an outdated server so just disconnect, unless we're insecure, then it's ok
		if (g_pCVar->FindVar("serverfilter")->GetBool()
		&& !g_pCVar->FindVar("ns_is_northstar_server")->GetBool()
		&& !g_pCVar->FindVar("ns_auth_allow_insecure")->GetBool())
		{
			Cbuf_AddText(
				Cbuf_GetCurrentPlayer(),
				"disconnect \"Validation failed, this server is either outdated or malicious.\"",
				cmd_source_t::kCommandSrcCode);
			Cbuf_Execute();

			return false;
		}

		return g_pCVar->FindVar("ns_is_northstar_server")->GetBool() == 0;
	}
};

inline VanillaCompatibility* g_pVanillaCompatibility;
