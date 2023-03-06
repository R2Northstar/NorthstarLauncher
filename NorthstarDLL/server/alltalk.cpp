#include "core/convar/convar.h"
#include "engine/r2engine.h"

size_t __fastcall ShouldAllowAlltalk()
{
	// this needs to return a 64 bit integer where 0 = true and 1 = false

	static ConVar* Cvar_sv_alltalk = R2::g_pCVar->FindVar("sv_alltalk");
	if (Cvar_sv_alltalk->GetBool())
		return 0;

	// lobby should default to alltalk, otherwise don't allow it
	return strcmp(R2::g_pGlobals->m_pMapName, "mp_lobby");
}

ON_DLL_LOAD_RELIESON("engine.dll", ServerAllTalk, ConVar, (CModule module))
{
	// replace strcmp function called in CClient::ProcessVoiceData with our own code that calls ShouldAllowAllTalk
	MemoryAddress base = module.Offset(0x1085FA);

	base.Patch("48 4B"); // mov rax, 64 bit int
	base.Offset(0x2).Patch((uint8_t*)&ShouldAllowAlltalk, sizeof(uintptr_t));
	base.Offset(0xA).Patch("FF D0"); // call rax

	// nop until compare (test eax, eax)
	base.Offset(0xC).NOP(0x7);
}
