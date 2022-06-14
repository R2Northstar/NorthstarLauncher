#include "pch.h"
#include "miscclientfixes.h"
#include "hookutils.h"
#include "dedicated.h"
#include "cvar.h"

typedef void* (*CrashingWeaponActivityFuncType)(void* a1);
CrashingWeaponActivityFuncType CrashingWeaponActivityFunc0;
CrashingWeaponActivityFuncType CrashingWeaponActivityFunc1;
typedef bool (*DevTextBufferDumpToFileType)(char* a1);
DevTextBufferDumpToFileType DevTextBufferDumpToFileClient;
typedef bool (*DevTextBufferWriteType)(char* a1);
DevTextBufferWriteType DevTextBufferWrite;

ConVar* cl_devtextbuffer_enable;

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

bool DevTextBufferDumpToFileHookClient(char* a1)
{
	// Prevent arbitrary file writes from squirrel
	if (!cl_devtextbuffer_enable->m_Value.m_nValue)
	{
		spdlog::info("Blocking dump to file \"{}\" from DevTextBuffer because cl_devtextbufferenable was 0", a1);
		return true;
	}
	return DevTextBufferDumpToFileClient(a1);
}

bool DevTextBufferWriteHook(char* a1)
{
	// Prevent arbitrary file writes from squirrel
	if (!cl_devtextbuffer_enable->m_Value.m_nValue)
	{
		spdlog::info("Blocking write to DevTextBuffer because cl_devtextbufferenable was 0");
		return true;
	}
	return DevTextBufferWrite(a1);
}

void InitialiseMiscClientFixes(HMODULE baseAddress)
{
	if (IsDedicated())
		return;

	HookEnabler hook;

	// these functions will occasionally pass a null pointer on respawn, unsure what causes this but seems easiest just to return null if
	// null, which seems to work fine fucking sucks this has to be fixed like this but unsure what exactly causes this serverside, breaks
	// vanilla compatibility to a degree tho will say i have about 0 clue what exactly these functions do, testing this it doesn't even seem
	// like they do much of anything i can see tbh
	ENABLER_CREATEHOOK(
		hook, (char*)baseAddress + 0x5A92D0, &CrashingWeaponActivityFunc0Hook, reinterpret_cast<LPVOID*>(&CrashingWeaponActivityFunc0));
	ENABLER_CREATEHOOK(
		hook, (char*)baseAddress + 0x5A9310, &CrashingWeaponActivityFunc1Hook, reinterpret_cast<LPVOID*>(&CrashingWeaponActivityFunc1));

	cl_devtextbuffer_enable = new ConVar("cl_devtextbufferenable", "0", 0, "Enables/Disables the use of DevTextBufferWrite on client.");

	ENABLER_CREATEHOOK(
		hook, (char*)baseAddress + 0x39D9F0, &DevTextBufferDumpToFileHookClient, reinterpret_cast<LPVOID*>(&DevTextBufferDumpToFileClient));
	ENABLER_CREATEHOOK(hook, (char*)baseAddress + 0x390CA0, &DevTextBufferWriteHook, reinterpret_cast<LPVOID*>(&DevTextBufferWrite));

	// experimental: allow cl_extrapolate to be enabled without cheats
	{
		void* ptr = (char*)baseAddress + 0x275F9D9;
		*((char*)ptr) = (char)0;
	}
}