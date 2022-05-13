#include "pch.h"
#include "hooks.h"
#include "scriptservertoclientstringcommand.h"
#include "squirrel.h"
#include "convar.h"
#include "concommand.h"

void ConCommand_ns_script_servertoclientstringcommand(const CCommand& arg)
{
	if (g_pClientSquirrel->sqvm &&
		g_pClientSquirrel->setupfunc("NSClientCodeCallback_RecievedServerToClientStringCommand") != SQRESULT_ERROR)
	{
		g_pClientSquirrel->pushstring(g_pClientSquirrel->sqvm2, arg.ArgS());
		g_pClientSquirrel->call(g_pClientSquirrel->sqvm2, 1); // todo: doesn't throw or log errors from within this, probably not great behaviour
	}
}

ON_DLL_LOAD_CLIENT_RELIESON("client.dll", ScriptServerToClientStringCommand, ClientSquirrel, [](HMODULE baseAddress)
{
	RegisterConCommand(
		"ns_script_servertoclientstringcommand",
		ConCommand_ns_script_servertoclientstringcommand,
		"",
		FCVAR_CLIENTDLL | FCVAR_SERVER_CAN_EXECUTE);
})