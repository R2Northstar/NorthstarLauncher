#include "pch.h"
#include "hooks.h"
#include "miscclientfixes.h"
#include "convar.h"
#include "hookutils.h"
#include "NSMem.h"

typedef void* (*CrashingWeaponActivityFuncType)(void* a1);
CrashingWeaponActivityFuncType CrashingWeaponActivityFunc0;
CrashingWeaponActivityFuncType CrashingWeaponActivityFunc1;

ConVar* Cvar_ns_cl_move_always_send;

void* CrashingWeaponActivityFunc0Hook(void* a1)
{
	// this return is safe, other functions that use this value seemingly dont care about it being null
	if (!a1)
		return 0;

	return CrashingWeaponActivityFunc0(a1);
}

void* CrashingWeaponActivityFunc1Hook(void* a1)
{
	// this return is safe, other functions that use this value seemingly dont care about it being null
	if (!a1)
		return 0;

	return CrashingWeaponActivityFunc1(a1);
}

ON_DLL_LOAD_CLIENT_RELIESON("client.dll", MiscClientFixes, ConVar, [](HMODULE baseAddress)
{
	HookEnabler hook;

	// these functions will occasionally pass a null pointer on respawn, unsure what causes this but seems easiest just to return null if
	// null, which seems to work fine fucking sucks this has to be fixed like this but unsure what exactly causes this serverside, breaks
	// vanilla compatibility to a degree tho will say i have about 0 clue what exactly these functions do, testing this it doesn't even seem
	// like they do much of anything i can see tbh
	ENABLER_CREATEHOOK(
		hook, (char*)baseAddress + 0x5A92D0, &CrashingWeaponActivityFunc0Hook, reinterpret_cast<LPVOID*>(&CrashingWeaponActivityFunc0));
	ENABLER_CREATEHOOK(
		hook, (char*)baseAddress + 0x5A9310, &CrashingWeaponActivityFunc1Hook, reinterpret_cast<LPVOID*>(&CrashingWeaponActivityFunc1));

	// experimental: allow cl_extrapolate to be enabled without cheats
	{
		void* ptr = (char*)baseAddress + 0x275F9D9;
		*((char*)ptr) = (char)0;
	}

	// allow demo recording on loopback
	NSMem::NOP((uintptr_t)GetModuleHandleA("engine.dll") + 0x8E1B1, 2);
	NSMem::NOP((uintptr_t)GetModuleHandleA("engine.dll") + 0x56CC3, 2);

	// change default values of demo cvars to enable them by default, but not autorecord
	// this is before Host_Init, the setvalue calls here will get overwritten by custom cfgs/launch options
	ConVar* Cvar_demo_enableDemos = g_pCVar->FindVar("demo_enabledemos");
	Cvar_demo_enableDemos->m_pszDefaultValue = "1";
	Cvar_demo_enableDemos->SetValue(true);

	ConVar* Cvar_demo_writeLocalFile = g_pCVar->FindVar("demo_writeLocalFile");
	Cvar_demo_writeLocalFile->m_pszDefaultValue = "1";
	Cvar_demo_writeLocalFile->SetValue(true);

	ConVar* Cvar_demo_autoRecord = g_pCVar->FindVar("demo_autoRecord");
	Cvar_demo_autoRecord->m_pszDefaultValue = "0";
	Cvar_demo_autoRecord->SetValue(false);
})