#include "pch.h"
#include "playlist.h"
#include "NSMem.h"
#include "concommand.h"
#include "convar.h"
#include "squirrel.h"

AUTOHOOK_INIT()

// use the R2 namespace for game funcs
namespace R2
{
	const char* (*GetCurrentPlaylistName)();
	void (*SetCurrentPlaylist)(const char* pPlaylistName);
	void (*SetPlaylistVarOverride)(const char* pVarName, const char* pValue);
	const char* (*GetCurrentPlaylistVar)(const char* pVarName, bool bUseOverrides);
} // namespace R2

ConVar* Cvar_ns_use_clc_SetPlaylistVarOverride;

AUTOHOOK(clc_SetPlaylistVarOverride__Process, engine.dll + 0x222180,
char,, (void* a1, void* a2))
{
	// the private_match playlist is the only situation where there should be any legitimate sending of this netmessage
	// todo: check map == mp_lobby here too
	if (!Cvar_ns_use_clc_SetPlaylistVarOverride->GetBool() || strcmp(R2::GetCurrentPlaylistName(), "private_match"))
		return 1;

	return clc_SetPlaylistVarOverride__Process(a1, a2);
}

AUTOHOOK(SetCurrentPlaylist, engine.dll + 0x18EB20,
void,, (const char* pPlaylistName))
{
	SetCurrentPlaylist(pPlaylistName);
	spdlog::info("Set playlist to {}", pPlaylistName);
}

AUTOHOOK(SetPlaylistVarOverride, engine.dll + 0x18ED00,
void,, (const char* pVarName, const char* pValue))
{
	if (strlen(pValue) >= 64)
		return;

	SetPlaylistVarOverride(pVarName, pValue);
}

AUTOHOOK(GetCurrentPlaylistVar, engine.dll + 0x18C680,
const char*,, (const char* pVarName, bool bUseOverrides))
{
	if (!bUseOverrides && !strcmp(pVarName, "max_players"))
		bUseOverrides = true;

	return GetCurrentPlaylistVar(pVarName, bUseOverrides);
}


AUTOHOOK(GetCurrentGamemodeMaxPlayers, engine.dll + 0x18C430,
int,, ())
{
	const char* pMaxPlayers = R2::GetCurrentPlaylistVar("max_players", 0);
	if (!pMaxPlayers)
		return GetCurrentGamemodeMaxPlayers();

	int iMaxPlayers = atoi(pMaxPlayers);
	return iMaxPlayers;
}


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

ON_DLL_LOAD_RELIESON("engine.dll", PlaylistHooks, ConCommand, (HMODULE baseAddress))
{
	AUTOHOOK_DISPATCH()

	R2::GetCurrentPlaylistName = (const char* (*)())((char*)baseAddress + 0x18C640);
	R2::SetCurrentPlaylist = (void (*)(const char*))((char*)baseAddress + 0x18EB20);
	R2::SetPlaylistVarOverride = (void (*)(const char*, const char*))((char*)baseAddress + 0x18ED00);
	R2::GetCurrentPlaylistVar = (const char* (*)(const char*, bool))((char*)baseAddress + 0x18C680);

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

	// patch to prevent clc_SetPlaylistVarOverride from being able to crash servers if we reach max overrides due to a call to Error (why is
	// this possible respawn, wtf) todo: add a warning for this
	NSMem::BytePatch((uintptr_t)baseAddress + 0x18ED8D, "C3");

	// patch to allow setplaylistvaroverride to be called before map init on dedicated and private match launched through the game
	NSMem::NOP((uintptr_t)baseAddress + 0x18ED17, 6);
}