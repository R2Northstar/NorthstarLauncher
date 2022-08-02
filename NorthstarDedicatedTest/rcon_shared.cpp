#include "pch.h"
#include "dedicated.h"
#include "concommand.h"
#include "convar.h"
#include "sv_rcon.h"
#include "cl_rcon.h"
#include "rcon_shared.h"

ConVar* CVar_rcon_address = nullptr;
ConVar* CVar_rcon_password = nullptr;
ConVar* CVar_sv_rcon_debug = nullptr;
ConVar* CVar_sv_rcon_maxfailures = nullptr;
ConVar* CVar_sv_rcon_maxignores = nullptr;
ConVar* CVar_sv_rcon_maxsockets = nullptr;
ConVar* CVar_sv_rcon_sendlogs = nullptr;
ConVar* CVar_sv_rcon_whitelist_address = nullptr;

/*
=====================
_RCON_CmdQuery_f

  Issues an RCON command to the
  RCON server.
=====================
*/
void _RCON_CmdQuery_f(const CCommand& args)
{
	if (IsDedicated())
	{
		return;
	}

	if (args.ArgC() < 2)
	{
		if (RCONClient()->IsInitialized() && !RCONClient()->IsConnected())
		{
			RCONClient()->Connect();
		}
	}
	else
	{
		if (!RCONClient()->IsInitialized())
		{
			spdlog::warn("Failed to issue command to RCON server: uninitialized\n");
			return;
		}
		else if (RCONClient()->IsConnected())
		{
			if (strcmp(args.Arg(1), "PASS") == 0) // Auth with RCON server using rcon_password ConVar value.
			{
				std::string svCmdQuery;
				if (args.ArgC() > 2)
				{
					svCmdQuery = RCONClient()->Serialize(args.Arg(2), "", cl_rcon::request_t::SERVERDATA_REQUEST_AUTH);
				}
				else // Use 'rcon_password' ConVar as password.
				{
					svCmdQuery = RCONClient()->Serialize(CVar_rcon_password->GetString(), "", cl_rcon::request_t::SERVERDATA_REQUEST_AUTH);
				}
				RCONClient()->Send(svCmdQuery);
				return;
			}
			else if (strcmp(args.Arg(1), "disconnect") == 0) // Disconnect from RCON server.
			{
				RCONClient()->Disconnect();
				return;
			}

			RCONClient()->Send(RCONClient()->Serialize(args.ArgS(), "", cl_rcon::request_t::SERVERDATA_REQUEST_EXECCOMMAND));
			return;
		}
		else
		{
			spdlog::warn("Failed to issue command to RCON server: unconnected\n");
			return;
		}
	}
}

/*
=====================
_RCON_Disconnect_f

  Disconnect from RCON server
=====================
*/
void _RCON_Disconnect_f(const CCommand& args)
{
	if (IsDedicated())
	{
		return;
	}
	if (RCONClient()->IsConnected())
	{
		RCONClient()->Disconnect();
		spdlog::info("User closed RCON connection");
	}
}

void InitializeRconSystems(HMODULE baseAddress)
{
	// TODO: RconPasswordChanged_f
	CVar_rcon_address = new ConVar(
		"rcon_address",
		"::",
		FCVAR_SERVER_CANNOT_QUERY | FCVAR_DONTRECORD | FCVAR_RELEASE,
		"Remote server access address.",
		false,
		0.f,
		false,
		0.f,
		nullptr);
	CVar_rcon_password = new ConVar(
		"rcon_password",
		"",
		FCVAR_SERVER_CANNOT_QUERY | FCVAR_DONTRECORD | FCVAR_RELEASE,
		"Remote server access password (rcon is disabled if empty).",
		false,
		0.f,
		false,
		0.f,
		nullptr);

	CVar_sv_rcon_debug =
		new ConVar("sv_rcon_debug", "0", FCVAR_RELEASE, "Show rcon debug information ( !slower! ).", false, 0.f, false, 0.f, nullptr);
	CVar_sv_rcon_maxfailures = new ConVar(
		"sv_rcon_maxfailures",
		"10",
		FCVAR_RELEASE,
		"Max number of times a user can fail rcon authentication before being banned.",
		false,
		0.f,
		false,
		0.f,
		nullptr);
	CVar_sv_rcon_maxignores = new ConVar(
		"sv_rcon_maxignores",
		"15",
		FCVAR_RELEASE,
		"Max number of times a user can ignore the no-auth message before being banned.",
		false,
		0.f,
		false,
		0.f,
		nullptr);
	CVar_sv_rcon_maxsockets = new ConVar(
		"sv_rcon_maxsockets",
		"32",
		FCVAR_RELEASE,
		"Max number of accepted sockets before the server starts closing redundant sockets.",
		false,
		0.f,
		false,
		0.f,
		nullptr);
	CVar_sv_rcon_sendlogs = new ConVar(
		"sv_rcon_sendlogs", "0", FCVAR_RELEASE, "If enabled, sends conlogs to connected netconsoles.", false, 0.f, false, 0.f, nullptr);
	CVar_sv_rcon_whitelist_address = new ConVar(
		"sv_rcon_whitelist_address",
		"",
		FCVAR_RELEASE,
		"This address is not considered a 'redundant' socket and will never be banned for failed authentications. Example: "
		"'::ffff:127.0.0.1'.",
		false,
		0.f,
		false,
		0.f,
		nullptr);
	RegisterConCommand(
		"rcon", _RCON_CmdQuery_f, "Forward RCON query to remote server. | Usage: rcon \"<query>\".", FCVAR_CLIENTDLL | FCVAR_RELEASE);

	RegisterConCommand("rcon_disconnect", _RCON_Disconnect_f, "Disconnect from RCON server.", FCVAR_CLIENTDLL | FCVAR_RELEASE);
}