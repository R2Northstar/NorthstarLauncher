#include "pch.h"
#include "convar.h"

// this replaces a string compare func
size_t __fastcall ShouldAllowAlltalk(const char* pMapName, const char* pDesiredMapName)
{
	// in vanilla, this just does strcmp(g_pMapName, "mp_lobby") and doesn't respect sv_alltalk
	static ConVar* Cvar_sv_alltalk = R2::g_pCVar->FindVar("sv_alltalk");
	if (Cvar_sv_alltalk->GetBool())
		return 0;

	return strcmp(pMapName, pDesiredMapName);
}

ON_DLL_LOAD_RELIESON("engine.dll", ServerAllTalk, ConVar, (CModule module)) 
{
	// replace strcmp function called in CClient::ProcessVoiceData with ShouldAllowAlltalk()
	MemoryAddress base = module.Offset(0x108608);
	// call ShouldAllowAlltalk
	MemoryAddress writeAddress((uint8_t*)&ShouldAllowAlltalk - module.Offset(0x10860D).m_nAddress);
	base.Offset(1).Patch((BYTE*)&writeAddress, 4);
}
