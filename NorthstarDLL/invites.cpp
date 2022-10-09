#include "pch.h"
#include "invites.h"
#include "squirrel.h"
#include "urihandler.h"

AUTOHOOK_INIT()

Invite* storedInvite;

std::string Invite::as_url()
{
	std::string url = "http://localhost:42069/invite?id={}&type={}";
	url = fmt::format(url, id, type);
	if (password != "")
	{
		//url.append(fmt::format("&password=", password));
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
	//g_pSquirrel<ScriptContext::UI>->schedule_call("NSInviteReceiveFromNative", id, password);
}

template <ScriptContext context> SQRESULT SQ_HasStoredInvite(HSquirrelVM* sqvm)
{
	g_pSquirrel<context>->pushbool(sqvm, true);
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT SQ_PullInviteFromNative(HSquirrelVM* sqvm)
{
	if (!storedInvite->active)
	{
		spdlog::warn("SQ_PullInvitesFromNative was called while no invite was stored");
		return SQRESULT_ERROR;
	}
	g_pSquirrel<context>->newtable(sqvm);

	g_pSquirrel<context>->pushstring(sqvm, "id", -1);
	g_pSquirrel<context>->pushstring(sqvm, storedInvite->id.c_str(), storedInvite->id.length());
	g_pSquirrel<context>->newslot(sqvm, -3, false);

	g_pSquirrel<context>->pushstring(sqvm, "type", -1);
	g_pSquirrel<context>->pushinteger(sqvm, storedInvite->type);
	g_pSquirrel<context>->newslot(sqvm, -3, false);

	g_pSquirrel<context>->pushstring(sqvm, "password", -1);
	g_pSquirrel<context>->pushstring(sqvm, storedInvite->password.c_str(), storedInvite->password.length());
	g_pSquirrel<context>->newslot(sqvm, -3, false);

	storedInvite->active = false;

	return SQRESULT_NOTNULL;
}

ON_DLL_LOAD_RELIESON("server.dll", InvitesSquirrel, ConCommand, (CModule module))
{
	AUTOHOOK_DISPATCH_MODULE(server.dll);

	g_pSquirrel<ScriptContext::UI>->AddFuncRegistration("bool", "NSHasStoredInvite", "", "", SQ_HasStoredInvite<ScriptContext::UI>);
	g_pSquirrel<ScriptContext::UI>->AddFuncRegistration(
		"table", "NSPullInviteFromNative", "", "", SQ_PullInviteFromNative<ScriptContext::UI>);
}
