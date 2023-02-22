#include "mods/modmanager.h"

AUTOHOOK_INIT()

OFFSET_STRUCT(WeaponDefinition)
{
	FIELD(5, bool bUnk); // this controls whether we reload script funcs
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

uint16_t* g_pnClientWeaponsLoaded;
GlobalWeaponDefs** g_ppClientWeaponDefs;

void (*ClientReparseWeapon)(WeaponDefinition* pWeapon);
void (*ClientReloadWeaponCallbacks)(int nWeaponIndex);
int (*ClientWeaponIndexByName)(const char* pWeaponName);

uint16_t* g_pnServerWeaponsLoaded;
GlobalWeaponDefs** g_ppServerWeaponDefs;

void (*ServerReparseWeapon)(WeaponDefinition* pWeapon);
void (*ServerReloadWeaponCallbacks)(int nWeaponIndex);
int (*ServerWeaponIndexByName)(const char* pWeaponName);

// used for passing client/server funcs/data/pointers to TryReloadWeapon
struct SidedWeaponReloadPointers
{
	// data pointers
	uint16_t* m_pnWeaponsLoaded;
	GlobalWeaponDefs** m_ppWeaponDefs;

	// funcs
	void (*m_fnReparseWeapon)(WeaponDefinition* pWeapon);
	void (*m_fnReloadWeaponCallbacks)(int nWeaponIndex);
	int (*m_fnWeaponIndexByName)(const char* pWeaponName);

	SidedWeaponReloadPointers(
		uint16_t* pnWeaponsLoaded,
		GlobalWeaponDefs** ppWeaponDefs,
		void (*fnReparseWeapon)(WeaponDefinition*),
		void (*fnReloadWeaponCallbacks)(int),
		int (*fnWeaponIndexByName)(const char*))
	{
		m_pnWeaponsLoaded = pnWeaponsLoaded;
		m_ppWeaponDefs = ppWeaponDefs;
		m_fnReparseWeapon = fnReparseWeapon;
		m_fnReloadWeaponCallbacks = fnReloadWeaponCallbacks;
		m_fnWeaponIndexByName = fnWeaponIndexByName;
	}
};

bool ModManager::TryReloadWeapon(const char* pWeaponName, const SidedWeaponReloadPointers* pReloadPointers)
{
	if (!m_AssetTypesToReload.setsWeaponSettings.contains(pWeaponName))
		return false; // don't reload

	int nWeaponIndex = pReloadPointers->m_fnWeaponIndexByName(pWeaponName);
	if (nWeaponIndex == -1) // weapon isn't loaded at all, no need to reload!
		return false;

	spdlog::info("ModManager::TryReloadWeapon reloading weapon {}", pWeaponName);

	WeaponDefinition* pWeapon = (*pReloadPointers->m_ppWeaponDefs)->m_Weapons->pWeaponDef;
	pReloadPointers->m_fnReparseWeapon(pWeapon);
	if (pWeapon->bUnk)
		pReloadPointers->m_fnReloadWeaponCallbacks(nWeaponIndex);

	m_AssetTypesToReload.setsWeaponSettings.erase(pWeaponName);
	return true;
}

// TODO: server implementation for this?
AUTOHOOK(ClientPrecacheWeaponFromStringtable, client.dll + 0x195A60,
bool, __fastcall, (void* a1, void* a2, void* a3, const char* pWeaponName))
{
	static SidedWeaponReloadPointers clientReloadPointers(
		g_pnClientWeaponsLoaded, g_ppClientWeaponDefs, ClientReparseWeapon, ClientReloadWeaponCallbacks, ClientWeaponIndexByName);

	if (g_pModManager->TryReloadWeapon(pWeaponName, &clientReloadPointers))
		return true;

	spdlog::info("PrecacheWeaponFromStringtable: {}", pWeaponName);
	return ClientPrecacheWeaponFromStringtable(a1, a2, a3, pWeaponName);
}

ON_DLL_LOAD_CLIENT("client.dll", ModReloadWeaponsClient, (CModule module))
{
	AUTOHOOK_DISPATCH()

	g_pnClientWeaponsLoaded = module.Offset(0xB33A02).As<uint16_t*>();
	g_ppClientWeaponDefs = module.Offset(0xB339E8).As<GlobalWeaponDefs**>();

	ClientReparseWeapon = module.Offset(0x3D2FB0).As<void(*)(WeaponDefinition*)>();
	ClientReloadWeaponCallbacks = module.Offset(0x3CE270).As<void(*)(int)>();
}
