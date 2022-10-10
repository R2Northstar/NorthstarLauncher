#include "pch.h"
#include "invites.h"
#include "squirrel.h"
#include "urihandler.h"
#include "masterserver.h"

AUTOHOOK_INIT()

Invite* storedInvite;

std::string Invite::as_url()
{
	std::string url = "http://localhost:42069/invite?id={}&type={}";
	url = fmt::format(url, id, type);
	if (password != "")
	{
		url.append(fmt::format("&password=", password));
	}
	return url;
}

void Invite::store()
{
	storedInvite->active = true;
	storedInvite->id = id;
	storedInvite->password = password;
	storedInvite->type = type;
	if (g_pSquirrel<ScriptContext::UI> == nullptr)
	{
		return;
	}

	if (g_pSquirrel<ScriptContext::UI>->m_pSQVM == nullptr)
	{
		return;
	}
	g_pSquirrel<ScriptContext::UI>->schedule_call("NSPullInvites");
}

template <ScriptContext context> SQRESULT SQ_HasStoredInvite(HSquirrelVM* sqvm)
{
	g_pSquirrel<context>->pushbool(sqvm, storedInvite->active);
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT SQ_LoadInviteAndJoin(HSquirrelVM* sqvm)
{
	g_pSquirrel<context>->pushbool(sqvm, storedInvite->active);
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT SQ_GenInviteTest(HSquirrelVM* sqvm)
{
	const char* id = g_pSquirrel<context>->getstring(sqvm, 1);
	storedInvite->active = true;
	storedInvite->id = id;
	storedInvite->password = "";
	storedInvite->type = InviteType::SERVER;
	g_pSquirrel<ScriptContext::UI>->schedule_call("NSPullInvites");
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT SQ_AllowShowInvite(HSquirrelVM* sqvm)
{
	g_pSquirrel<context>->pushbool(sqvm, g_pMasterServerManager->m_sOwnClientAuthToken[0] != 0);
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT SQ_DeclineInvite(HSquirrelVM* sqvm)
{
	storedInvite->active = false;
	return SQRESULT_NOTNULL;
}


template <ScriptContext context> SQRESULT SQ_ParseInvite(HSquirrelVM* sqvm)
{
	std::string invite_str = g_pSquirrel<context>->getstring(sqvm, 1);
	auto maybe_invite = parseURI(invite_str);
	if (!maybe_invite)
	{
		g_pSquirrel<context>->raiseerror(sqvm, "Could not parse invite string");
		return SQRESULT_ERROR;
	}
	Invite invite = maybe_invite.value();
	invite.store();
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT SQ_PullInviteFromNative(HSquirrelVM* sqvm)
{
	if (!storedInvite->active)
	{
		spdlog::warn("SQ_PullInvitesFromNative was called while no invite was stored");
		g_pSquirrel<context>->raiseerror(sqvm, "SQ_PullInvitesFromNative was called while no invite was stored");
		return SQRESULT_ERROR;
	}

	std::optional<RemoteServerInfo> server = std::nullopt;
	int index = 0;
	for (auto& s : g_pMasterServerManager->m_vRemoteServers)
	{
		if (s.id == storedInvite->id)
		{
			server = s;

			break;
		}
		index++;
	}
	if (!server)
	{
		spdlog::warn("Tried parsing invite with invalid or expired server id");
		g_pSquirrel<context>->raiseerror(sqvm, "Tried parsing invite with invalid or expired server id");
		return SQRESULT_ERROR;
	}

	g_pSquirrel<context>->newtable(sqvm);

	g_pSquirrel<context>->pushstring(sqvm, "index", -1);
	g_pSquirrel<context>->pushinteger(sqvm, index);
	g_pSquirrel<context>->newslot(sqvm, -3, false);

	g_pSquirrel<context>->pushstring(sqvm, "id", -1);
	g_pSquirrel<context>->pushstring(sqvm, storedInvite->id.c_str(), storedInvite->id.length());
	g_pSquirrel<context>->newslot(sqvm, -3, false);

	g_pSquirrel<context>->pushstring(sqvm, "type", -1);
	g_pSquirrel<context>->pushinteger(sqvm, storedInvite->type);
	g_pSquirrel<context>->newslot(sqvm, -3, false);

	g_pSquirrel<context>->pushstring(sqvm, "password", -1);
	g_pSquirrel<context>->pushstring(sqvm, storedInvite->password.c_str(), storedInvite->password.length());
	g_pSquirrel<context>->newslot(sqvm, -3, false);

	g_pSquirrel<context>->pushstring(sqvm, "name", -1);
	g_pSquirrel<context>->pushstring(sqvm, server->name, -1);
	g_pSquirrel<context>->newslot(sqvm, -3, false);

	g_pSquirrel<context>->pushstring(sqvm, "requires_password", -1);
	g_pSquirrel<context>->pushbool(sqvm, server->requiresPassword);
	g_pSquirrel<context>->newslot(sqvm, -3, false);

	storedInvite->active = false;

	return SQRESULT_NOTNULL;
}

ON_DLL_LOAD_CLIENT_RELIESON("client.dll", ClientSquirrelInvites, ClientSquirrel, (CModule module))
{
	AUTOHOOK_DISPATCH()

	g_pSquirrel<ScriptContext::UI>->AddFuncRegistration("bool", "NSHasStoredInvite", "", "", SQ_HasStoredInvite<ScriptContext::UI>);
	g_pSquirrel<ScriptContext::UI>->AddFuncRegistration("void", "NSDeclineInvite", "", "", SQ_DeclineInvite<ScriptContext::UI>);
	g_pSquirrel<ScriptContext::UI>->AddFuncRegistration("bool", "NSAllowShowInvite", "", "", SQ_AllowShowInvite<ScriptContext::UI>);
	g_pSquirrel<ScriptContext::UI>->AddFuncRegistration("void", "inv", "string id", "", SQ_GenInviteTest<ScriptContext::UI>);
	g_pSquirrel<ScriptContext::UI>->AddFuncRegistration("void", "NSParseInvite", "string invite", "", SQ_ParseInvite<ScriptContext::UI>);
	g_pSquirrel<ScriptContext::UI>->AddFuncRegistration(
		"table", "NSPullInviteFromNative", "", "", SQ_PullInviteFromNative<ScriptContext::UI>);
}
