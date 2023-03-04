#include "mods/modmanager.h"

AUTOHOOK_INIT()

// these are all used in their respective precacheweapon functions to determine whether to load these files
// just set these to make game reload assets during load!
VAR_AT(client.dll + 0x23EF0C5, bool*, g_pbClientHasLoadedDamageFlags);
VAR_AT(client.dll + 0x23EF0C6, bool*, g_pbClientHasLoadedWeaponSprings);
VAR_AT(client.dll + 0x23EF0C7, bool*, g_pbClientHasLoadedWeaponAmmoSuckBehaviours);
VAR_AT(client.dll + 0x23EF0C4, bool*, g_pbClientHasLoadedWeaponADSPulls);

VAR_AT(server.dll + 0x160B474, bool*, g_pbServerHasLoadedDamageFlags);
VAR_AT(server.dll + 0x160B475, bool*, g_pbServerHasLoadedWeaponSprings);
VAR_AT(server.dll + 0x160B476, bool*, g_pbServerHasLoadedWeaponAmmoSuckBehaviours);
VAR_AT(server.dll + 0x160B477, bool*, g_pbServerHasLoadedWeaponADSPulls);

void ModManager::DeferredReloadDamageFlags()
{
	*g_pbClientHasLoadedDamageFlags = false;
	*g_pbServerHasLoadedDamageFlags = false;
}

void ModManager::DeferredReloadWeaponSprings()
{
	*g_pbClientHasLoadedWeaponSprings = false;
	*g_pbServerHasLoadedWeaponSprings = false;
}

void ModManager::DeferredReloadAmmoSuckBehaviours()
{
	*g_pbClientHasLoadedWeaponAmmoSuckBehaviours = false;
	*g_pbServerHasLoadedWeaponAmmoSuckBehaviours = false;
}

void ModManager::DeferredReloadADSPulls()
{
	*g_pbClientHasLoadedWeaponADSPulls = false;
	*g_pbServerHasLoadedWeaponADSPulls = false;
}


ON_DLL_LOAD_CLIENT("client.dll", ClientModReloadWeaponsMisc, (CModule module))
{
	AUTOHOOK_DISPATCH_MODULE(client.dll)
}
