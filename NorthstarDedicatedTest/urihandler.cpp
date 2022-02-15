#include <Shlwapi.h>
#include <iostream>
#include <windows.h>
#include <filesystem>
#include "pch.h"
#include "squirrel.h"
#include "gameutils.h"
#include <thread>
#include <chrono>
#include <TlHelp32.h>
#include <sstream>
#include <fstream>
#include "masterserver.h"
#include "clientauthhooks.h"
#include "urihandler.h"
#include "base64.h"

enum InviteType
{
	Server = 0,
	Party = 1,
};

InviteType storedInviteType = InviteType::Server;
std::string storedServerId;
std::string storedPassword;

bool hasStoredURI = false;
bool isOnMainMenu = true; // For some fucking reason, calling OpenDialog from c++ on the main menu crashes, so we use this workaround

/// URI Handler
/// This file is responsible for handling everything related to URI parsing and invites and stuff
/// The URI format for northstar is as follows:
/// northstar://(server/party)@lobbyId[:password]
/// [:password] is an optional base64 encoded string
/// We use WinAPI's NamedPipes for inter-process communication

HANDLE initPipe()
{
	// Create a pipe to send data
	HANDLE pipe = CreateNamedPipe(
		L"\\\\.\\pipe\\northstar", // name of the pipe
		PIPE_ACCESS_DUPLEX,		   // duplex for ease of use
		PIPE_TYPE_BYTE,			   // send data as a byte stream
		1,						   // only allow 1 instance of this pipe
		0,						   // no outbound buffer
		0,						   // no inbound buffer
		0,						   // use default wait time
		NULL					   // use default security attributes
	);
	return pipe;
}

void parseURI(std::string uriString) {
	int uriOffset = URIProtocolName.length();
	if (uriString.find(URIProtocolName) != std::string::npos)
	{
		uriString = uriString.substr(uriOffset, uriString.length() - uriOffset); // -1 to remove a trailing slash -_-
	}
	std::string password;
	spdlog::info("Parsing URI: {}", uriString.c_str());

	int atLocation = uriString.find("@");
	if (atLocation == std::string::npos)
	{
		spdlog::info("Invalid or malformed URI. Returning early.");
		return;
	}
	std::string invitetype = uriString.substr(0, atLocation);
	if (invitetype == "server"){storedInviteType = InviteType::Server;}
	else if (invitetype == "party"){storedInviteType = InviteType::Party;}
	else
	{
		spdlog::info("Invalid or unknown invite type \"{}\". Returning early.", invitetype);
		return;
	}
	uriString = uriString.substr(atLocation + 1);
	int sep = uriString.find(":");
	if (sep != std::string::npos)
	{
		storedServerId = uriString.substr(0, sep);
		storedPassword = base64_decode(uriString.substr(sep + 1)); // This crashes when the input b64 is invalid, whatever
	}
	else
	{
		storedServerId = uriString;
		storedPassword = "";
	}
	spdlog::info("================================");
	spdlog::info("Parsed URI: ");
	spdlog::info("Invite type: {}", invitetype);
	spdlog::info("ID: {}", storedServerId.c_str());
	spdlog::info("password: {}", storedPassword.c_str());

	hasStoredURI = true;
}

void HandleAcceptedInvite()
{
	if (storedInviteType == InviteType::Server) // Party invites are currently unsupported
	{
		spdlog::info("Handling Accepted Invite");
		if (!g_MasterServerManager->m_bOriginAuthWithMasterServerDone && GetAgreedToSendToken() != 2)
		{
			// if player has agreed to send token and we aren't already authing, try to auth
			if (GetAgreedToSendToken() == 1 && !g_MasterServerManager->m_bOriginAuthWithMasterServerInProgress)
				g_MasterServerManager->AuthenticateOriginWithMasterServer(g_LocalPlayerUserID, g_LocalPlayerOriginToken, false);
		
			// invalidate key so auth will fail
			*g_LocalPlayerOriginToken = 0;
		}

		spdlog::info("{}, {}, {}, {}", g_LocalPlayerUserID, g_MasterServerManager->m_ownClientAuthToken, (char*)storedServerId.c_str(),
		(char*)storedPassword.c_str());
		while (g_MasterServerManager->m_bOriginAuthWithMasterServerInProgress)
		{
			Sleep(10);
		}
		g_MasterServerManager->AuthenticateWithServer(
			g_LocalPlayerUserID, g_MasterServerManager->m_ownClientAuthToken, (char*)storedServerId.c_str(), (char*)storedPassword.c_str(),
			false);
		RemoteServerConnectionInfo info = g_MasterServerManager->m_pendingConnectionInfo;
		spdlog::info("connect {}.{}.{}.{}:{}", info.ip.S_un.S_un_b.s_b1, info.ip.S_un.S_un_b.s_b2, info.ip.S_un.S_un_b.s_b3,
		info.ip.S_un.S_un_b.s_b4,info.port);
		Cbuf_AddText(Cbuf_GetCurrentPlayer(), fmt::format("serverfilter {}", info.authToken).c_str(), cmd_source_t::kCommandSrcCode);
		Cbuf_AddText(
			Cbuf_GetCurrentPlayer(),
			fmt::format(
				"connect {}.{}.{}.{}:{}", info.ip.S_un.S_un_b.s_b1, info.ip.S_un.S_un_b.s_b2, info.ip.S_un.S_un_b.s_b3,
				info.ip.S_un.S_un_b.s_b4, info.port)
				.c_str(),
			cmd_source_t::kCommandSrcCode);

		g_MasterServerManager->m_hasPendingConnectionInfo = false;
	}
}

SQRESULT SQ_HasStoredURI(void* sqvm)
{
	ClientSq_pushbool(sqvm, hasStoredURI);
	return SQRESULT_NOTNULL;
}

SQRESULT SQ_AcceptInvite(void* sqvm)
{
	HandleAcceptedInvite();
	hasStoredURI = false;
	return SQRESULT_NOTNULL;
}

SQRESULT SQ_DeclineInvite(void* sqvm)
{
	hasStoredURI = false;
	return SQRESULT_NOTNULL;
}

SQRESULT SQ_SetOnMainMenu(void* sqvm)
{
	isOnMainMenu = ClientSq_getbool(sqvm, 1);
	return SQRESULT_NOTNULL;
}

SQRESULT SQ_RemoveStoredURI(void* sqvm)
{
	hasStoredURI = false;
	return SQRESULT_NOTNULL;
}

void StartUriHandler()
{
	spdlog::info("Creating an instance of a named pipe...\n");

	HANDLE pipe = initPipe();

	if (pipe == NULL || pipe == INVALID_HANDLE_VALUE)
	{
		spdlog::info("Failed to create outbound pipe instance.\n");
	}

	spdlog::info("Waiting for a client to connect to the pipe...\n");

	BOOL result = ConnectNamedPipe(pipe, NULL);

	spdlog::info("Connected Pipe generated.\n");
	if (!result)
	{
		spdlog::info("Failed to make connection on named pipe.\n");
		CloseHandle(pipe); // close the pipe
	}

	bool shouldRecreate = false;

	while (1)
	{
		{
			if (shouldRecreate)
			{
				pipe = initPipe();

				if (pipe == NULL || pipe == INVALID_HANDLE_VALUE)
				{
					// what the fuck
				}

				BOOL result = ConnectNamedPipe(pipe, NULL);
				if (!result)
				{
					CloseHandle(pipe); // close the pipe
				}
			}

			// The read operation will block until there is data to read
			wchar_t buffer[128]; // original

			DWORD numBytesRead = 0;
			BOOL result = ReadFile(
				pipe,
				buffer,				   // the data from the pipe will be put here
				127 * sizeof(wchar_t), // number of bytes allocated
				&numBytesRead,		   // this will store number of bytes actually read
				NULL				   // not using overlapped IO
			);

			if (result)
			{
				buffer[numBytesRead / sizeof(wchar_t)] = '\0'; // null terminate the string
				std::wstring w = std::wstring(buffer);
				std::string message = std::string(w.begin(), w.end());
				spdlog::info("Message: ");
				spdlog::info(message.c_str());
				spdlog::info("\n");
				parseURI(message);
				if (!isOnMainMenu)
				{
					g_UISquirrelManager->ExecuteCode("ShowURIDialog()");
				}
			}
			CloseHandle(pipe);
			shouldRecreate = true;
		}
	}

	// Close the pipe (automatically disconnects client too)
	spdlog::info("Closing handle #2\n");
	CloseHandle(pipe);

	spdlog::info("Done.\n");
}

void InitialiseURIStuff(HMODULE baseAddress)
{
	std::thread thread1(std::ref(StartUriHandler)); // Start NamedPipe handler
	thread1.detach(); // Detach from it so it doesn't block main thread

	g_UISquirrelManager->AddFuncRegistration("bool", "NSHasStoredURI", "", "", SQ_HasStoredURI);
	g_UISquirrelManager->AddFuncRegistration("bool", "NSRemoveStoredURI", "", "", SQ_RemoveStoredURI);
	g_UISquirrelManager->AddFuncRegistration("void", "NSAcceptInvite", "", "", SQ_AcceptInvite);
	g_UISquirrelManager->AddFuncRegistration("void", "NSDeclineInvite", "", "", SQ_DeclineInvite);
	g_UISquirrelManager->AddFuncRegistration("void", "NSSetOnMainMenu", "bool onMainMenu", "", SQ_SetOnMainMenu);
}