#include "pch.h"
#include "dedicated.h"
#include "tier0.h"
#include "playlist.h"
#include "r2engine.h"
#include "hoststate.h"
#include "serverauthentication.h"
#include "masterserver.h"
#include "printcommand.h"

AUTOHOOK_INIT()

using namespace R2;

bool IsDedicatedServer()
{
	static bool result = strstr(GetCommandLineA(), "-dedicated");
	return result;
}

// CDedidcatedExports defs
struct CDedicatedExports; // forward declare

typedef void (*DedicatedSys_PrintfType)(CDedicatedExports* dedicated, const char* msg);
typedef void (*DedicatedRunServerType)(CDedicatedExports* dedicated);

// would've liked to just do this as a class but have not been able to get it to work
struct CDedicatedExports
{
	void* vtable; // because it's easier, we just set this to &this, since CDedicatedExports has no props we care about other than funcs

	char unused[56];

	DedicatedSys_PrintfType Sys_Printf;
	DedicatedRunServerType RunServer;
};

void Sys_Printf(CDedicatedExports* dedicated, const char* msg)
{
	spdlog::info("[DEDICATED SERVER] {}", msg);
}

typedef void (*CHostState__InitType)(CHostState* self);

void RunServer(CDedicatedExports* dedicated)
{
	spdlog::info("CDedicatedExports::RunServer(): starting");
	spdlog::info(Tier0::CommandLine()->GetCmdLine());

	// initialise engine
	g_pEngine->Frame();

	// add +map if not present
	// don't manually execute this from cbuf as users may have it in their startup args anyway, easier just to run from stuffcmds if present
	if (!Tier0::CommandLine()->CheckParm("+map"))
		Tier0::CommandLine()->AppendParm("+map", g_pCVar->FindVar("match_defaultMap")->GetString());

	// re-run commandline
	Cbuf_AddText(Cbuf_GetCurrentPlayer(), "stuffcmds", cmd_source_t::kCommandSrcCode);
	Cbuf_Execute();

	// get tickinterval
	ConVar* Cvar_base_tickinterval_mp = g_pCVar->FindVar("base_tickinterval_mp");

	// main loop
	double frameTitle = 0;
	while (g_pEngine->m_nQuitting == EngineQuitState::QUIT_NOTQUITTING)
	{
		double frameStart = Tier0::Plat_FloatTime();
		g_pEngine->Frame();

		// only update the title after at least 500ms since the last update
		if ((frameStart - frameTitle) > 0.5)
		{
			frameTitle = frameStart;

			// this way of getting playercount/maxplayers honestly really sucks, but not got any other methods of doing it rn
			const char* maxPlayers = GetCurrentPlaylistVar("max_players", true);
			if (!maxPlayers)
				maxPlayers = "6";

			SetConsoleTitleA(fmt::format(
								 "{} - {} {}/{} players ({})",
								 g_pMasterServerManager->m_sUnicodeServerName,
								 g_pHostState->m_levelName,
								 g_pServerAuthentication->m_PlayerAuthenticationData.size(),
								 maxPlayers,
								 GetCurrentPlaylistName())
								 .c_str());
		}

		std::this_thread::sleep_for(std::chrono::duration<double, std::ratio<1>>(
			Cvar_base_tickinterval_mp->GetFloat() - fmin(Tier0::Plat_FloatTime() - frameStart, 0.25)));
	}
}

AUTOHOOK(IsGameActiveWindow, engine.dll + 0x1CDC80, 
bool,, ())
{
	return true;	
}

HANDLE consoleInputThreadHandle = NULL;

DWORD WINAPI ConsoleInputThread(PVOID pThreadParameter)
{
	while (!g_pEngine || !g_pHostState || g_pHostState->m_iCurrentState != HostState_t::HS_RUN)
		Sleep(1000);

	// Bind stdin to receive console input.
	FILE* fp = nullptr;
	freopen_s(&fp, "CONIN$", "r", stdin);

	spdlog::info("Ready to receive console commands.");

	{
		// Process console input
		std::string input;
		while (g_pEngine && g_pEngine->m_nQuitting == EngineQuitState::QUIT_NOTQUITTING && std::getline(std::cin, input))
		{
			input += "\n";
			Cbuf_AddText(Cbuf_GetCurrentPlayer(), input.c_str(), cmd_source_t::kCommandSrcCode);
			TryPrintCvarHelpForCommand(input.c_str());
		}
	}

	return 0;
}

#include "NSMem.h"
ON_DLL_LOAD_DEDI("engine.dll", DedicatedServer, (HMODULE engineAddress))
{
	spdlog::info("InitialiseDedicated");

	AUTOHOOK_DISPATCH_MODULE("engine.dll")

	uintptr_t ea = (uintptr_t)engineAddress;

	// Host_Init
	// prevent a particle init that relies on client dll
	NSMem::NOP(ea + 0x156799, 5);

	// Host_Init
	// don't call Key_Init to avoid loading some extra rsons from rpak (will be necessary to boot if we ever wanna disable rpaks entirely on dedi)
	NSMem::NOP(ea + 0x1565B0, 5);

	{
		// CModAppSystemGroup::Create
		// force the engine into dedicated mode by changing the first comparison to IsServerOnly to an assignment
		auto ptr = ea + 0x1C4EBD;

		// cmp => mov
		NSMem::BytePatch(ptr + 1, "C6 87");

		// 00 => 01
		NSMem::BytePatch(ptr + 7, "01");
	}
	

	// Some init that i'm not sure of that crashes
	// nop the call to it
	NSMem::NOP(ea + 0x156A63, 5);

	// runframeserver
	// nop some access violations
	NSMem::NOP(ea + 0x159819, 17);

	NSMem::NOP(ea + 0x156B4C, 7);

	// previously patched these, took me a couple weeks to figure out they were the issue
	// removing these will mess up register state when this function is over, so we'll write HS_RUN to the wrong address
	// so uhh, don't do that
	// NSMem::NOP(ea + 0x156B4C + 7, 8);
	NSMem::NOP(ea + 0x156B4C + 15, 9);

	// HostState_State_NewGame
	// nop an access violation
	NSMem::NOP(ea + 0xB934C, 9);

	// CEngineAPI::Connect
	// remove call to Shader_Connect
	NSMem::NOP(ea + 0x1C4D7D, 5);

	// Host_Init
	// remove call to ui loading stuff
	NSMem::NOP(ea + 0x156595, 5);

	// some function that gets called from RunFrameServer
	// nop a function that makes requests to stryder, this will eventually access violation if left alone and isn't necessary anyway
	NSMem::NOP(ea + 0x15A0BB, 5);

	// RunFrameServer
	// nop a function that access violations
	NSMem::NOP(ea + 0x159BF3, 5);

	// func that checks if origin is inited
	// always return 1
	NSMem::BytePatch(
		ea + 0x183B70,
		{
			0xB0,
			0x01, // mov al,01
			0xC3 // ret
		});

	// HostState_State_ChangeLevel
	// nop clientinterface call
	NSMem::NOP(ea + 0x1552ED, 16);

	// HostState_State_ChangeLevel
	// nop clientinterface call
	NSMem::NOP(ea + 0x155363, 16);

	// IVideoMode::CreateGameWindow
	// nop call to ShowWindow
	NSMem::NOP(ea + 0x1CD146, 5);


	CDedicatedExports* dedicatedExports = new CDedicatedExports;
	dedicatedExports->vtable = dedicatedExports;
	dedicatedExports->Sys_Printf = Sys_Printf;
	dedicatedExports->RunServer = RunServer;

	CDedicatedExports** exports = (CDedicatedExports**)((char*)engineAddress + 0x13F0B668);
	*exports = dedicatedExports;

	// extra potential patches:
	// nop engine.dll+1c67d1 and +1c67d8 to skip videomode creategamewindow
	// also look into launcher.dll+d381, seems to cause renderthread to get made
	// this crashes HARD if no window which makes sense tbh
	// also look into materialsystem + 5B344 since it seems to be the base of all the renderthread stuff

	// big note: datatable gets registered in window creation
	// make sure it still gets registered

	// add cmdline args that are good for dedi
	Tier0::CommandLine()->AppendParm("-nomenuvid", 0);
	Tier0::CommandLine()->AppendParm("-nosound", 0);
	Tier0::CommandLine()->AppendParm("-windowed", 0);
	Tier0::CommandLine()->AppendParm("-nomessagebox", 0);
	Tier0::CommandLine()->AppendParm("+host_preload_shaders", "0");
	Tier0::CommandLine()->AppendParm("+net_usesocketsforloopback", "1");

	// Disable Quick Edit mode to reduce chance of user unintentionally hanging their server by selecting something.
	if (!Tier0::CommandLine()->CheckParm("-bringbackquickedit"))
	{
		HANDLE stdIn = GetStdHandle(STD_INPUT_HANDLE);
		DWORD mode = 0;

		if (GetConsoleMode(stdIn, &mode))
		{
			if (mode & ENABLE_QUICK_EDIT_MODE)
			{
				mode &= ~ENABLE_QUICK_EDIT_MODE;
				mode &= ~ENABLE_MOUSE_INPUT;

				mode |= ENABLE_PROCESSED_INPUT;

				SetConsoleMode(stdIn, mode);
			}
		}
	}
	else
		spdlog::info("Quick Edit enabled by user request");

	// create console input thread
	if (!Tier0::CommandLine()->CheckParm("-noconsoleinput"))
		consoleInputThreadHandle = CreateThread(0, 0, ConsoleInputThread, 0, 0, NULL);
	else
		spdlog::info("Console input disabled by user request");
}

ON_DLL_LOAD_DEDI("tier0.dll", DedicatedServerOrigin, (HMODULE baseAddress))
{
	// disable origin on dedicated
	// for any big ea lawyers, this can't be used to play the game without origin, game will throw a fit if you try to do anything without
	// an origin id as a client for dedi it's fine though, game doesn't care if origin is disabled as long as there's only a server

	NSMem::BytePatch((uintptr_t)GetProcAddress(GetModuleHandleA("tier0.dll"), "Tier0_InitOrigin"),
	{
		0xC3 // ret
	});
}

AUTOHOOK(PrintFatalSquirrelError, server.dll + 0x794D0, 
void, , (void* sqvm))
{
	PrintFatalSquirrelError(sqvm);
	g_pEngine->m_nQuitting = EngineQuitState::QUIT_TODESKTOP;
}

ON_DLL_LOAD_DEDI("server.dll", DedicatedServerGameDLL, (HMODULE baseAddress))
{
	AUTOHOOK_DISPATCH_MODULE("server.dll")
}