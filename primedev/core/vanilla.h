#pragma once

/// Determines if we are in vanilla-compatibility mode.
/// In this mode we shouldn't auth with Atlas, which prevents users from joining a
/// non-trusted server. This means that we can unrestrict client/server commands
/// as well as various other small changes for compatibility
class VanillaCompatibility
{
public:
	bool GetVanillaCompatibility()
	{
		return (g_pCVar->FindVar("ns_is_northstar_server")->GetBool() == 0);
	}
};

inline VanillaCompatibility* g_pVanillaCompatibility;
