#include "mods/modmanager.h"
#include "dedicated/dedicated.h"
#include "engine/r2engine.h"

AUTOHOOK_INIT()

// these are all used in their respective precacheweapon functions to determine whether to load these files
// just set these to make game reload assets during load!
VAR_AT(client.dll + 0x23EF0C5, bool*, g_pbClientHasLoadedDamageFlags);
VAR_AT(client.dll + 0x23EF0C6, bool*, g_pbClientHasLoadedWeaponSprings);
VAR_AT(client.dll + 0x23EF0C7, bool*, g_pbClientHasLoadedWeaponAmmoSuckBehaviours);
VAR_AT(client.dll + 0x23EF0C4, bool*, g_pbClientHasLoadedWeaponADSPulls);

FUNCTION_AT(client.dll + 0x3CDDF0, void,, ClientReparseDamageFlags, ()); // requires g_pbClientHasLoadedDamageFlags to be false
FUNCTION_AT(client.dll + 0x3CE5B0, void,, ClientReparseWeaponSprings, ());
FUNCTION_AT(client.dll + 0x3CE060, void,, ClientReparseWeaponAmmoSuckBehaviours, ());
FUNCTION_AT(client.dll + 0x3CD700, void,, ClientReparseWeaponADSPulls, ());

VAR_AT(server.dll + 0x160B474, bool*, g_pbServerHasLoadedDamageFlags);
VAR_AT(server.dll + 0x160B475, bool*, g_pbServerHasLoadedWeaponSprings);
VAR_AT(server.dll + 0x160B476, bool*, g_pbServerHasLoadedWeaponAmmoSuckBehaviours);
VAR_AT(server.dll + 0x160B477, bool*, g_pbServerHasLoadedWeaponADSPulls);

FUNCTION_AT(client.dll + 0x6CC9F0, void,, ServerReparseDamageFlags, ()); // requires g_pbClientHasLoadedDamageFlags to be false
FUNCTION_AT(client.dll + 0x6CD1C0, void,, ServerReparseWeaponSprings, ());
FUNCTION_AT(client.dll + 0x6CCC70, void,, ServerReparseWeaponAmmoSuckBehaviours, ());
FUNCTION_AT(client.dll + 0x6CC300, void,, ServerReparseWeaponADSPulls, ());

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

void ModManager::TryImmediateReloadDamageFlags()
{
	if (!IsDedicatedServer())
	{
		*g_pbClientHasLoadedDamageFlags = false;
		if (g_pSquirrel<ScriptContext::CLIENT>->m_pSQVM)
			ClientReparseDamageFlags();
	}

	*g_pbServerHasLoadedDamageFlags = false;
	if (*R2::g_pServerState == R2::ss_active)
		ServerReparseDamageFlags();
}

void ModManager::TryImmediateReloadWeaponSprings()
{
	if (!IsDedicatedServer())
	{
		*g_pbClientHasLoadedWeaponSprings = false;
		if (g_pSquirrel<ScriptContext::CLIENT>->m_pSQVM)
			ClientReparseWeaponSprings();
	}

	*g_pbServerHasLoadedWeaponSprings = false;
	if (*R2::g_pServerState == R2::ss_active)
		ServerReparseWeaponSprings();
}

void ModManager::TryImmediateReloadAmmoSuckBehaviours()
{
	if (!IsDedicatedServer())
	{
		*g_pbClientHasLoadedWeaponAmmoSuckBehaviours = false;
		if (g_pSquirrel<ScriptContext::CLIENT>->m_pSQVM)
			ClientReparseWeaponAmmoSuckBehaviours();
	}

	*g_pbServerHasLoadedWeaponAmmoSuckBehaviours = false;
	if (*R2::g_pServerState == R2::ss_active)
		ServerReparseWeaponAmmoSuckBehaviours();
}

void ModManager::TryImmediateReloadADSPulls()
{
	if (!IsDedicatedServer())
	{
		*g_pbClientHasLoadedWeaponADSPulls = false;
		if (g_pSquirrel<ScriptContext::CLIENT>->m_pSQVM)
			ClientReparseWeaponADSPulls();
	}

	*g_pbServerHasLoadedWeaponADSPulls = false;
	if (*R2::g_pServerState == R2::ss_active)
		ServerReparseWeaponADSPulls();
}

ON_DLL_LOAD_CLIENT("client.dll", ClientModReloadWeaponsMisc, (CModule module))
{
	AUTOHOOK_DISPATCH_MODULE(client.dll)
}

ON_DLL_LOAD_CLIENT("server.dll", ServerModReloadWeaponsMisc, (CModule module))
{
	AUTOHOOK_DISPATCH_MODULE(server.dll)
}
