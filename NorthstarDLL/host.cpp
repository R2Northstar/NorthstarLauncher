#include "pch.h"
#include "convar.h"
#include "modmanager.h"
#include "printcommand.h"
#include "printmaps.h"
#include "misccommands.h"
#include "r2engine.h"
#include "tier0.h"
#include "splash.h"
#include <chrono>

AUTOHOOK_INIT()

// clang-format off
AUTOHOOK(CDedicatedAppSystemGroupStarter, engine.dll + 0x1C78F0,
void, __fastcall, (void* a1))
// clang-format on
{
	SetSplashMessage("Starting game", 8);
	return CDedicatedAppSystemGroupStarter(a1);
}

static bool hasPassed = false;

// clang-format off
AUTOHOOK(BikInterfaceLoader, engine.dll + 0x46320,
void, __fastcall, (__int64 a1, __int64 a2))
// clang-format on
{
	if (!hasPassed)
	{
		hasPassed = true;
		SetSplashMessage("Starting game engine", 7);
	}
	return BikInterfaceLoader(a1, a2);
}

// clang-format off
AUTOHOOK(Host_Init, engine.dll + 0x155EA0,
void, __fastcall, (bool bDedicated))
// clang-format on
{
	SetSplashMessage("Loading Main Menu", 9);
	std::thread test(
		[]()
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		SetSplashMessage("Done", 10, true);
		});
	test.detach();
	spdlog::info("Host_Init()");
	Host_Init(bDedicated);

	FixupCvarFlags();

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
