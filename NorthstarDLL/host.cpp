#include "pch.h"
#include "convar.h"
#include "modmanager.h"
#include "printcommand.h"
#include "printmaps.h"
#include "r2engine.h"
#include "tier0.h"

AUTOHOOK_INIT()

// clang-format off
AUTOHOOK(Host_Init, engine.dll + 0x155EA0,
void, __fastcall, (bool bDedicated))
// clang-format on
{
	spdlog::info("Host_Init()");
	Host_Init(bDedicated);

    if (Tier0::CommandLine()->CheckParm("-allowdevcvars"))
    {
		// get all mod convars
		std::vector<std::string> vModConvarNames;
		for (auto& mod : g_pModManager->m_LoadedMods)
			for (auto& cvar : mod.ConVars)
				vModConvarNames.push_back(cvar->Name);

	    // strip hidden and devonly cvar flags
	    int iNumCvarsAltered = 0;
	    for (auto& pair : R2::g_pCVar->DumpToMap())
	    {
	    	// don't remove from mod cvars
	    	if (std::find(vModConvarNames.begin(), vModConvarNames.end(), pair.second->m_pszName) != vModConvarNames.end())
	    		continue;

	    	// strip flags
	    	int flags = pair.second->GetFlags();
	    	if (flags & FCVAR_DEVELOPMENTONLY)
	    	{
	    		flags &= ~FCVAR_DEVELOPMENTONLY;
	    		iNumCvarsAltered++;
	    	}

	    	if (flags & FCVAR_HIDDEN)
	    	{
	    		flags &= ~FCVAR_HIDDEN;
	    		iNumCvarsAltered++;
	    	}

	    	pair.second->m_nFlags = flags;
	    }

		spdlog::info("Removed {} hidden/devonly cvar flags", iNumCvarsAltered);
    }

	// make servers able to run disconnect on clients
	R2::g_pCVar->FindCommand("disconnect")->m_nFlags |= FCVAR_SERVER_CAN_EXECUTE;

	R2::g_pCVar->FindCommand("migrateme")->m_nFlags &= ~FCVAR_SERVER_CAN_EXECUTE;

	// make clients able to run status and ping
	R2::g_pCVar->FindCommand("status")->m_nFlags |= FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS;
	R2::g_pCVar->FindCommand("ping")->m_nFlags |= FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS;

	// need to initialise these after host_init since they do stuff to preexisting concommands/convars without being client/server specific
	InitialiseCommandPrint();
	InitialiseMapsPrint();

	// client/server autoexecs on necessary platforms
	// dedi needs autoexec_ns_server on boot, while non-dedi will run it on on listen server start
	if (bDedicated)
		R2::Cbuf_AddText(R2::Cbuf_GetCurrentPlayer(), "exec autoexec_ns_server", R2::cmd_source_t::kCommandSrcCode);
	else
		R2::Cbuf_AddText(R2::Cbuf_GetCurrentPlayer(), "exec autoexec_ns_client", R2::cmd_source_t::kCommandSrcCode);
}

ON_DLL_LOAD("engine.dll", Host_Init, (CModule module))
{
	AUTOHOOK_DISPATCH()
}
