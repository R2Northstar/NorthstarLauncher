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

std::string storedServerId;
std::string storedPassword;
bool handledStoreURI = false;
bool isOnMainMenu = true; // For some fucking reason, calling OpenDialog from c++ on the main menu crashes, so we use this workaround

HANDLE initPipe()
{
	// Create a pipe to send data
	HANDLE pipe = CreateNamedPipe(
		L"\\\\.\\pipe\\northstar", // name of the pipe
		PIPE_ACCESS_DUPLEX,		   // 1-way pipe -- send only
		PIPE_TYPE_BYTE,			   // send data as a byte stream
		1,						   // only allow 1 instance of this pipe
		0,						   // no outbound buffer
		0,						   // no inbound buffer
		0,						   // use default wait time
		NULL					   // use default security attributes
	);
	return pipe;
}

void setURIFromLaunch(const char* serverId, const char* password, bool hasURI)
{
	storedServerId = serverId;
	storedPassword = password;
	handledStoreURI = !hasURI;
}

void TestFunc(const char* serverId, const char* password)
{
	// Cbuf_AddText(Cbuf_GetCurrentPlayer(), fmt::format("connect \"emmam.nl:37016\"").c_str(), cmd_source_t::kCommandSrcCode);
	// TODO: need to add a thing to auth with ms if not already

	if (!g_MasterServerManager->m_bOriginAuthWithMasterServerDone && GetAgreedToSendToken() != 2)
	{
		// if player has agreed to send token and we aren't already authing, try to auth
		if (GetAgreedToSendToken() == 1 && !g_MasterServerManager->m_bOriginAuthWithMasterServerInProgress)
			g_MasterServerManager->AuthenticateOriginWithMasterServer(g_LocalPlayerUserID, g_LocalPlayerOriginToken, true);

		// invalidate key so auth will fail
		*g_LocalPlayerOriginToken = 0;
	}

	spdlog::info("88888888888888888888888888888888888888");
	spdlog::info("88888888888888888888888888888888888888");
	spdlog::info("88888888888888888888888888888888888888");
	spdlog::info("88888888888888888888888888888888888888");
	spdlog::info("88888888888888888888888888888888888888");
	spdlog::info("{}, {}, {}, {}", g_LocalPlayerUserID, g_MasterServerManager->m_ownClientAuthToken, (char*)serverId, (char*)password);
	spdlog::info("88888888888888888888888888888888888888");
	spdlog::info("88888888888888888888888888888888888888");
	spdlog::info("88888888888888888888888888888888888888");
	spdlog::info("88888888888888888888888888888888888888");
	spdlog::info("88888888888888888888888888888888888888");
	g_MasterServerManager->AuthenticateWithServer(
		g_LocalPlayerUserID, g_MasterServerManager->m_ownClientAuthToken, (char*)serverId, (char*)password, false);
	RemoteServerConnectionInfo info = g_MasterServerManager->m_pendingConnectionInfo;
	spdlog::info("5555555555555555555555555555555555555555555555555555555555555");
	spdlog::info("5555555555555555555555555555555555555555555555555555555555555");
	spdlog::info("5555555555555555555555555555555555555555555555555555555555555");
	spdlog::info("5555555555555555555555555555555555555555555555555555555555555");
	spdlog::info("5555555555555555555555555555555555555555555555555555555555555");
	spdlog::info(
		"connect {}.{}.{}.{}:{}", info.ip.S_un.S_un_b.s_b1, info.ip.S_un.S_un_b.s_b2, info.ip.S_un.S_un_b.s_b3, info.ip.S_un.S_un_b.s_b4,
		info.port);
	spdlog::info("5555555555555555555555555555555555555555555555555555555555555");
	spdlog::info("5555555555555555555555555555555555555555555555555555555555555");
	spdlog::info("5555555555555555555555555555555555555555555555555555555555555");
	spdlog::info("5555555555555555555555555555555555555555555555555555555555555");
	spdlog::info("5555555555555555555555555555555555555555555555555555555555555");
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

SQRESULT SQ_HandleURI(void* sqvm)
{
	g_UISquirrelManager->ExecuteCode("ShowInviteDialog()");
	handledStoreURI = true;
	return SQRESULT_NOTNULL;
}

SQRESULT SQ_HasStoredURI(void* sqvm)
{
	ClientSq_pushbool(sqvm, !handledStoreURI);
	return SQRESULT_NOTNULL;
}

SQRESULT SQ_AcceptInvite(void* sqvm)
{
	TestFunc(storedServerId.c_str(), storedPassword.c_str());
	handledStoreURI = true;
	return SQRESULT_NOTNULL;
}

SQRESULT SQ_DeclineInvite(void* sqvm)
{
	handledStoreURI = true;
	return SQRESULT_NOTNULL;
}

SQRESULT SQ_SetOnMainMenu(void* sqvm)
{
	isOnMainMenu = ClientSq_getbool(sqvm, 1);
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
		// look up error code here using GetLastError()
		CloseHandle(pipe); // close the pipe
	}

	spdlog::info("Here.\n");

	bool shouldRecreate = false;

	while (1)
	{
		{
			if (shouldRecreate)
			{
				pipe = initPipe();

				if (pipe == NULL || pipe == INVALID_HANDLE_VALUE)
				{
					// look up error code here using GetLastError()
				}

				BOOL result = ConnectNamedPipe(pipe, NULL);
				if (!result)
				{
					// look up error code here using GetLastError()
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
				// Convert wchar_t buffer to a char*
				size_t origsize = wcslen(buffer) + 1;
				const size_t newsize = 100;
				size_t convertedChars = 0;
				char nstring[newsize];
				wcstombs_s(&convertedChars, nstring, origsize, buffer, _TRUNCATE);
				spdlog::info(nstring);
				spdlog::info("\n");
				std::string stripped = std::string(nstring);
				std::string password;
				spdlog::info("stripped: {}", stripped.c_str());
				int sep = stripped.find(":");
				if (sep > 0)
				{
					password = stripped.substr(sep + 1);
					stripped = stripped.substr(0, sep);
				}
				spdlog::info("Displaying info: ");
				storedServerId = stripped;
				storedPassword = password;
				handledStoreURI = false;
				if (!isOnMainMenu)
				{
					g_UISquirrelManager->ExecuteCode("OpenInviteDialog()");
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
	std::thread thread1(std::ref(StartUriHandler));
	thread1.detach();
	g_UISquirrelManager->AddFuncRegistration("void", "NSHandleStoredURI", "", "", SQ_HandleURI);
	g_UISquirrelManager->AddFuncRegistration("bool", "NSHasStoredURI", "", "", SQ_HasStoredURI);
	g_UISquirrelManager->AddFuncRegistration("void", "NSAcceptInvite", "", "", SQ_AcceptInvite);
	g_UISquirrelManager->AddFuncRegistration("void", "NSDeclineInvite", "", "", SQ_DeclineInvite);
	g_UISquirrelManager->AddFuncRegistration("void", "NSSetOnMainMenu", "bool onMainMenu", "", SQ_SetOnMainMenu);
}