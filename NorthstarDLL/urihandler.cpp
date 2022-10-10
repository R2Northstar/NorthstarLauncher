#include "pch.h"
#include "urihandler.h"
#include "invites.h"
#include "squirrel.h"

#include "base64.h"
#include <shellapi.h>

AUTOHOOK_INIT()

URIHandler* g_pURIHandler;

/// URI Handler
/// This file is responsible for handling everything related to URI parsing and invites and stuff
/// The URI format for northstar is as follows:
/// northstar://(server/party)@lobbyId[:password]
/// [:password] is an optional base64 encoded string

void URIHandler::StartServer()
{
	spdlog::info("Starting URI Server");
	if (m_bIsRunning)
	{
		spdlog::info("URIHandler::StartServer was called while server is already running");
		return;
	}

	m_bIsRunning = true;

	std::thread server(
		[this]
		{
			m_URIServer.Get(
				"/invite",
				[](const httplib::Request& req, httplib::Response& response)
				{
					spdlog::info("\n\n\nGOT REQ\n\n\n");
					Invite invite = {};
					if (!req.has_param("id"))
					{
						response.set_content("{\"succes\": false, \"reason\": \"missing field ID\"}", "application/json");
						return;
					}
					if (!req.has_param("type"))
					{
						response.set_content("{\"succes\": false, \"reason\": \"missing field TYPE\"}", "application/json");
						return;
					}
					invite.id = req.get_param_value("id");
					invite.active = true;
					if (req.get_param_value("type") == "server")
					{
						invite.type = InviteType::SERVER;
					}
					else if (req.get_param_value("type") == "party")
					{
						invite.type = InviteType::PARTY;
					}
					if (req.has_param("password"))
					{
						invite.password = base64_decode(req.get_param_value("password"));
					}
					response.set_content("{\"succes\": true}", "application/json");
					invite.store();
				});
			m_URIServer.Get(
				"/ping", [](const httplib::Request& req, httplib::Response& response) { response.set_content("1", "text/plaintext"); });
			spdlog::info("test123");
			bool test = m_URIServer.listen("0.0.0.0", 42069);
			spdlog::info("Was succesfull? {}", test);
		});
	server.detach();
}

bool URIHandler::CheckInstall()
{
	if (Cvar_ns_dont_ask_install_urihandler->GetBool())
	{
		return true;
	}
	HKEY subKey = nullptr;
	LONG result = RegOpenKeyEx(HKEY_CLASSES_ROOT, L"northstar", 0, KEY_READ, &subKey);
	std::ofstream file;
	if (result == ERROR_SUCCESS)
	{
		return true;
	}
	return false;
}

template <ScriptContext context> SQRESULT SQ_CheckURIHandlerInstall(HSquirrelVM* sqvm)
{
	g_pSquirrel<context>->pushbool(sqvm, g_pURIHandler->CheckInstall());
	return SQRESULT_NOTNULL;
}

template <ScriptContext context> SQRESULT SQ_DoInstallURIHandler(HSquirrelVM* sqvm)
{
	wchar_t buffer[_MAX_PATH];
	GetModuleFileNameW(NULL, buffer, _MAX_PATH); // Get full executable path
	std::wstring w = std::wstring(buffer);
	std::string exepath = std::string(w.begin(), w.end()); // Convert from wstring to string

	ShellExecute(
		NULL,
		L"runas",
		w.c_str(),
		L" -addurihandler",
		NULL, // default dir
		SW_SHOWNORMAL);
	return SQRESULT_NOTNULL;
}

ON_DLL_LOAD_CLIENT_RELIESON("client.dll", ClientSquirrelURIHandler, ClientSquirrelInvites, (CModule module))
{
	AUTOHOOK_DISPATCH()

	g_pSquirrel<ScriptContext::UI>->AddFuncRegistration(
		"bool", "NSCheckURIHandlerInstall", "", "", SQ_CheckURIHandlerInstall<ScriptContext::UI>);
	g_pSquirrel<ScriptContext::UI>->AddFuncRegistration("void", "NSDoInstallURIHandler", "", "", SQ_DoInstallURIHandler<ScriptContext::UI>);
}
