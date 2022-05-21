#include "pch.h"
#include "convar.h"
#include "NSMem.h"

ON_DLL_LOAD_CLIENT_RELIESON("client.dll", DemoFixes, ConVar, [](HMODULE baseAddress)
{
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