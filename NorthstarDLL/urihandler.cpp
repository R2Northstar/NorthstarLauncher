#include "pch.h"
#include "urihandler.h"
#include "invites.h"

#include "base64.h"

URIHandler* g_pURIHandler;

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
						invite.password = "base64";
						//invite.password = base64_decode(req.get_param_value("password"));
					}
					response.set_content("{\"succes\": true}", "application/json");
				});
			m_URIServer.Get(
				"/ping", [](const httplib::Request& req, httplib::Response& response) { response.set_content("1", "text/plaintext"); });
			spdlog::info("test123");
			bool test = m_URIServer.listen("0.0.0.0", 42069);
			spdlog::info("Was succesfull? {}", test);
		});
	server.detach();
}
