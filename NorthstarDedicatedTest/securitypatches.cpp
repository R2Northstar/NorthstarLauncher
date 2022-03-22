#include "pch.h"
#include "securitypatches.h"
#include "hookutils.h"
#include "concommand.h"
#include "gameutils.h"
#include "convar.h"

typedef bool (*IsValveModType)();
IsValveModType IsValveMod;

bool IsValveModHook()
{
	// basically: by default r2 isn't set as a valve mod, meaning that m_bRestrictServerCommands is false
	// this is HORRIBLE for security, because it means servers can run arbitrary concommands on clients
	// especially since we have script commands this could theoretically be awful
	return !CommandLine()->CheckParm("-norestrictservercommands");
}

#include "NSMem.h"
void InitialiseClientEngineSecurityPatches(HMODULE baseAddress)
{
	uintptr_t ba = (uintptr_t)baseAddress;

	HookEnabler hook;

	// note: this could break some things
	ENABLER_CREATEHOOK(hook, (char*)baseAddress + 0x1C6360, &IsValveModHook, reinterpret_cast<LPVOID*>(&IsValveMod));

	// patches to make commands run from client/ui script still work
	// note: this is likely preventable in a nicer way? test prolly
	{
		NSMem::BytePatch(ba + 0x4FB65, {0xEB, 0x11});
	}

	{
		NSMem::BytePatch(ba + 0x4FBAC, {0xEB, 0x16});
	}

	// byte patches to patch concommands that this messes up that we need
	{
		// disconnect concommand
		int val = *(int*)(ba + 0x5ADA2D) | FCVAR_SERVER_CAN_EXECUTE;
		NSMem::BytePatch(ba + 0x5ADA2D, (BYTE*)&val, sizeof(int));
	}
}