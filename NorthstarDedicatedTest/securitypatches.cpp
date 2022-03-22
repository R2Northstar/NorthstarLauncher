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

typedef bool (*SVC_CmdKeyValues__ReadFromBufferType)(void* a1, void* a2);
SVC_CmdKeyValues__ReadFromBufferType SVC_CmdKeyValues__ReadFromBuffer;
// never parse server=>client keyvalues for clientcommandkeyvalues
bool SVC_CmdKeyValues__ReadFromBufferHook(void* a1, void* a2) { return false; }

void InitialiseClientEngineSecurityPatches(HMODULE baseAddress)
{
	HookEnabler hook;

	// note: this could break some things
	ENABLER_CREATEHOOK(hook, (char*)baseAddress + 0x1C6360, &IsValveModHook, reinterpret_cast<LPVOID*>(&IsValveMod));
	ENABLER_CREATEHOOK(hook, (char*)baseAddress + 0x222E70, &SVC_CmdKeyValues__ReadFromBufferHook, reinterpret_cast<LPVOID*>(&SVC_CmdKeyValues__ReadFromBuffer));

	// patches to make commands run from client/ui script still work
	// note: this is likely preventable in a nicer way? test prolly
	{
		void* ptr = (char*)baseAddress + 0x4FB65;
		TempReadWrite rw(ptr);

		*((char*)ptr) = (char)0xEB;
		*((char*)ptr + 1) = (char)0x11;
	}

	{
		void* ptr = (char*)baseAddress + 0x4FBAC;
		TempReadWrite rw(ptr);

		*((char*)ptr) = (char)0xEB;
		*((char*)ptr + 1) = (char)0x16;
	}

	// byte patches to patch concommands that this messes up that we need
	{
		// disconnect concommand
		void* ptr = (char*)baseAddress + 0x5ADA2D;
		TempReadWrite rw(ptr);

		*((int*)ptr) |= FCVAR_SERVER_CAN_EXECUTE;
	}
}