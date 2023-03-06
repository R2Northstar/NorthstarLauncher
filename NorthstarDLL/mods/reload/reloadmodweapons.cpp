#include "mods/modmanager.h"

AUTOHOOK_INIT()

OFFSET_STRUCT(WeaponDefinition)
{
	FIELD(5, bool bReloadScriptFuncs);
	FIELD(6, char pWeaponName[]); // this probably has a max length but i do not know what it is
};

OFFSET_STRUCT(GlobalWeaponDefs)
{
	// each entry is 24 bytes, but no clue what the bytes after the def are, so just ignore atm
	// need the full struct so we iterate properly
	OFFSET_STRUCT(WeaponDefContainer)
	{
		STRUCT_SIZE(24);
		WeaponDefinition* pWeaponDef;
	};

	FIELD(16, WeaponDefContainer m_Weapons[]);
};

VAR_AT(client.dll + 0xB33A02, uint16_t*, g_pnClientWeaponsLoaded);
VAR_AT(client.dll + 0xB339E8, GlobalWeaponDefs**, g_ppClientWeaponDefs);

FUNCTION_AT(client.dll + 0x3D2FB0, void,, ClientReparseWeapon, (WeaponDefinition* pWeapon));
FUNCTION_AT(client.dll + 0x3CE270, void,, ClientReloadWeaponCallbacks, (int nWeaponIndex));

std::unordered_set<std::string> setsClientWeaponsToReload;
std::unordered_set<std::string> setsServerWeaponsToReload;

// used for passing client/server funcs/data/pointers to TryReloadWeapon
struct SidedWeaponReloadPointers
{
	// data pointers
	uint16_t* m_pnWeaponsLoaded;
	GlobalWeaponDefs** m_ppWeaponDefs;

	std::unordered_set<std::string>* m_psetsWeaponsToReload;

	// funcs
	void (*m_fnReparseWeapon)(WeaponDefinition* pWeapon);
	void (*m_fnReloadWeaponCallbacks)(int nWeaponIndex);

	SidedWeaponReloadPointers(
		uint16_t* pnWeaponsLoaded,
		GlobalWeaponDefs** ppWeaponDefs,
		std::unordered_set<std::string>* psetsWeaponsToReload,
		void (*fnReparseWeapon)(WeaponDefinition*),
		void (*fnReloadWeaponCallbacks)(int))
	{
		m_pnWeaponsLoaded = pnWeaponsLoaded;
		m_ppWeaponDefs = ppWeaponDefs;
		m_psetsWeaponsToReload = psetsWeaponsToReload;
		m_fnReparseWeapon = fnReparseWeapon;
		m_fnReloadWeaponCallbacks = fnReloadWeaponCallbacks;
	}
};


int WeaponIndexByName(const char* pWeaponName, const SidedWeaponReloadPointers* pReloadPointers)
{
	for (int i = 0; i < *pReloadPointers->m_pnWeaponsLoaded; i++)
	{
		const WeaponDefinition* pWeapon = (*pReloadPointers->m_ppWeaponDefs)->m_Weapons[i].pWeaponDef;
		if (!strcmp(pWeapon->pWeaponName, pWeaponName))
			return i;
	}

	return -1;
}

void ModManager::DeferredReloadWeapons(const std::unordered_set<std::string> setsWeapons)
{
	// if there's still weapons that need reloading, then keep them, just reload the new stuff
	for (const std::string& sWeapon : setsWeapons)
	{
		setsClientWeaponsToReload.insert(sWeapon);
		setsServerWeaponsToReload.insert(sWeapon);
	}
}

bool ModManager::TryReloadWeapon(const char* pWeaponName, const SidedWeaponReloadPointers* pReloadPointers)
{
	if (!pReloadPointers->m_psetsWeaponsToReload->contains(pWeaponName))
		return false; // don't reload

	int nWeaponIndex = WeaponIndexByName(pWeaponName, pReloadPointers);
	if (nWeaponIndex == -1) // weapon isn't loaded at all, no need to reload!
		return false;

	spdlog::info("ModManager::TryReloadWeapon reloading weapon {}", pWeaponName);

	WeaponDefinition* pWeapon = (*pReloadPointers->m_ppWeaponDefs)->m_Weapons[nWeaponIndex].pWeaponDef;
	bool bReloadScriptFuncs = pWeapon->bReloadScriptFuncs; // this is reset after reparse
	pReloadPointers->m_fnReparseWeapon(pWeapon);
	if (bReloadScriptFuncs)
		pReloadPointers->m_fnReloadWeaponCallbacks(nWeaponIndex);

	pReloadPointers->m_psetsWeaponsToReload->erase(pWeaponName);
	return true;
}

// TODO: server implementation for this?
// clang-format off
AUTOHOOK(ClientPrecacheWeaponFromStringtable, client.dll + 0x195A60,
bool, __fastcall, (void* a1, void* a2, void* a3, const char* pWeaponName))
// clang-format on
{
	static SidedWeaponReloadPointers clientReloadPointers(
		g_pnClientWeaponsLoaded, g_ppClientWeaponDefs, &setsClientWeaponsToReload, ClientReparseWeapon, ClientReloadWeaponCallbacks);

	if (g_pModManager->TryReloadWeapon(pWeaponName, &clientReloadPointers))
		return true;

	spdlog::info("PrecacheWeaponFromStringtable: {}", pWeaponName);
	return ClientPrecacheWeaponFromStringtable(a1, a2, a3, pWeaponName);
}

ON_DLL_LOAD_CLIENT("client.dll", ModReloadWeaponsClient, (CModule module))
{
	AUTOHOOK_DISPATCH_MODULE(server.dll)
}
