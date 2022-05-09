#include "pch.h"
#include "playlist.h"
#include "NSMem.h"
#include "hooks.h"
#include "concommand.h"
#include "convar.h"
#include "gameutils.h"
#include "hookutils.h"
#include "squirrel.h"

// use the R2 namespace for game funcs
namespace R2
{
	GetCurrentPlaylistNameType GetCurrentPlaylistName;
	SetCurrentPlaylistType SetCurrentPlaylist;
	SetPlaylistVarOverrideType SetPlaylistVarOverride;
	GetCurrentPlaylistVarType GetCurrentPlaylistVar;
} // namespace R2

void ConCommand_playlist(const CCommand& args)
{
	if (args.ArgC() < 2)
		return;

	R2::SetCurrentPlaylist(args.Arg(1));
}

void ConCommand_setplaylistvaroverride(const CCommand& args)
{
	if (args.ArgC() < 3)
		return;

	for (int i = 1; i < args.ArgC(); i += 2)
		R2::SetPlaylistVarOverride(args.Arg(i), args.Arg(i + 1));
}

ConVar* Cvar_ns_use_clc_SetPlaylistVarOverride;

typedef char (*Onclc_SetPlaylistVarOverrideType)(void* a1, void* a2);
Onclc_SetPlaylistVarOverrideType Onclc_SetPlaylistVarOverride;
char Onclc_SetPlaylistVarOverrideHook(void* a1, void* a2)
{
	// the private_match playlist is the only situation where there should be any legitimate sending of this netmessage
	// todo: check mp_lobby here too
	if (!Cvar_ns_use_clc_SetPlaylistVarOverride->GetBool() || strcmp(R2::GetCurrentPlaylistName(), "private_match"))
		return 1;

	return Onclc_SetPlaylistVarOverride(a1, a2);
}

void SetPlaylistVarOverrideHook(const char* varName, const char* value)
{
	if (strlen(value) >= 64)
		return;

	R2::SetPlaylistVarOverride(varName, value);
}

const char* GetCurrentPlaylistVarHook(const char* varName, bool useOverrides)
{
	if (!useOverrides && !strcmp(varName, "max_players"))
		useOverrides = true;

	return R2::GetCurrentPlaylistVar(varName, useOverrides);
}

typedef int (*GetCurrentGamemodeMaxPlayersType)();
GetCurrentGamemodeMaxPlayersType GetCurrentGamemodeMaxPlayers;
int GetCurrentGamemodeMaxPlayersHook()
{
	char* maxPlayersStr = R2::GetCurrentPlaylistVar("max_players", 0);
	if (!maxPlayersStr)
		return GetCurrentGamemodeMaxPlayers();

	int maxPlayers = atoi(maxPlayersStr);
	return maxPlayers;
}

ON_DLL_LOAD_RELIESON("engine.dll", PlaylistHooks, ConCommand, [](HMODULE baseAddress)
{
	// playlist is the name of the command on respawn servers, but we already use setplaylist so can't get rid of it
	RegisterConCommand("playlist", ConCommand_playlist, "Sets the current playlist", FCVAR_NONE);
	RegisterConCommand("setplaylist", ConCommand_playlist, "Sets the current playlist", FCVAR_NONE);
	RegisterConCommand("setplaylistvaroverrides", ConCommand_setplaylistvaroverride, "sets a playlist var override", FCVAR_NONE);
	
	// note: clc_SetPlaylistVarOverride is pretty insecure, since it allows for entirely arbitrary playlist var overrides to be sent to the
	// server, this is somewhat restricted on custom servers to prevent it being done outside of private matches, but ideally it should be
	// disabled altogether, since the custom menus won't use it anyway this should only really be accepted if you want vanilla client
	// compatibility
	Cvar_ns_use_clc_SetPlaylistVarOverride = new ConVar(
		"ns_use_clc_SetPlaylistVarOverride", "0", FCVAR_GAMEDLL, "Whether the server should accept clc_SetPlaylistVarOverride messages");

	R2::GetCurrentPlaylistName = (R2::GetCurrentPlaylistNameType)((char*)baseAddress + 0x18C640);
	R2::SetCurrentPlaylist = (R2::SetCurrentPlaylistType)((char*)baseAddress + 0x18EB20);
	R2::SetPlaylistVarOverride = (R2::SetPlaylistVarOverrideType)((char*)baseAddress + 0x18ED00);
	R2::GetCurrentPlaylistVar = (R2::GetCurrentPlaylistVarType)((char*)baseAddress + 0x18C680);

	HookEnabler hook;
	ENABLER_CREATEHOOK(
		hook, (char*)baseAddress + 0x222180, &Onclc_SetPlaylistVarOverrideHook, reinterpret_cast<LPVOID*>(&Onclc_SetPlaylistVarOverride));
	ENABLER_CREATEHOOK(
		hook, (char*)baseAddress + 0x18ED00, &SetPlaylistVarOverrideHook, reinterpret_cast<LPVOID*>(&R2::SetPlaylistVarOverride));
	ENABLER_CREATEHOOK(
		hook, (char*)baseAddress + 0x18C680, &GetCurrentPlaylistVarHook, reinterpret_cast<LPVOID*>(&R2::GetCurrentPlaylistVar));
	ENABLER_CREATEHOOK(
		hook,
		(char*)baseAddress + 0x18C430,
		&GetCurrentGamemodeMaxPlayersHook,
		reinterpret_cast<LPVOID*>(&GetCurrentGamemodeMaxPlayers));

	uintptr_t ba = (uintptr_t)baseAddress;

	// patch to prevent clc_SetPlaylistVarOverride from being able to crash servers if we reach max overrides due to a call to Error (why is
	// this possible respawn, wtf) todo: add a warning for this
	NSMem::BytePatch(ba + 0x18ED8D, "C3");

	// patch to allow setplaylistvaroverride to be called before map init on dedicated and private match launched through the game
	NSMem::NOP(ba + 0x18ED17, 6);
})