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
		// since serverfilter is required for vanilla and is set for ns auth you can use this to check if we are in working with vanilla,
		// might be better to use a replicated cvar but lazy, you do it
		return std::string(g_pCVar->FindVar("serverfilter")->GetString()).empty();
	}

private:
	bool m_bIsVanillaCompatible = false;
};

inline VanillaCompatibility* g_pVanillaCompatibility;
