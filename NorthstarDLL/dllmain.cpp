#include "pch.h"
#include "main.h"
#include "logging.h"
#include "crashhandler.h"
#include "memalloc.h"
#include "nsprefix.h"
#include "plugin_abi.h"
#include "plugins.h"
#include "version.h"
#include "pch.h"
#include "urihandler.h"

#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/error/en.h"

#include <string.h>
#include <filesystem>

#include "invites.h"
#include "squirrel.h"
#include "wininfo.h"

// https://forums.codeguru.com/showthread.php?270538-How-to-check-for-open-ports-on-host-machine-(winsock-)
bool is_port_open(int port)
{
	// udp uses SOCK_DGRAM, tcp uses SOCK_STREAM
	SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
	if (s != INVALID_SOCKET)
	{
		SOCKADDR_IN sd;
		sd.sin_family = AF_INET;
		sd.sin_port = htons(port);
		sd.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

		if (bind(s, (PSOCKADDR)&sd, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
			return false;
	}
	else
		return false;

	return true;
}

bool CheckURI()
{
	bool hasURIString = strstr(GetCommandLineA(), "northstar://");
	if (!hasURIString)
	{
		return false;
	}

	std::string cla = GetCommandLineA();
	int uriOffset = cla.find(URIProtocolName) + URIProtocolName.length();
	std::string message = cla.substr(uriOffset, cla.length() - uriOffset - 1); // -1 to remove a trailing slash -_-
	auto maybe_invite = parseURI(message);
	if (!maybe_invite)
	{
		return false;
	}
	auto invite = maybe_invite.value();
				
	std::string url = invite.as_local_request();
	CURL* curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
	curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 1000L);
	CURLcode result = curl_easy_perform(curl);
	if (result == CURLE_OPERATION_TIMEDOUT) // For some reason this is used when it cant find the resource
	{
		invite.store();
		return true;
	}
	else
	{
		exit(0);
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	g_NorthstarModule = hModule;
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}


bool InitialiseNorthstar()
{
	static bool bInitialised = false;
	if (bInitialised)
		return false;

	bInitialised = true;

	InitialiseLogging();
	CreateLogFiles();

	InitialiseNorthstarPrefix();

	g_pURIHandler = new URIHandler();
	storedInvite = new Invite();
	CheckURI();
	g_pURIHandler->StartServer();

	g_pPluginManager = new PluginManager();
	g_pPluginManager->LoadPlugins();

	InitialiseVersion();

	// Fix some users' failure to connect to respawn datacenters
	SetEnvironmentVariableA("OPENSSL_ia32cap", "~0x200000200000000");

	curl_global_init_mem(CURL_GLOBAL_DEFAULT, _malloc_base, _free_base, _realloc_base, _strdup_base, _calloc_base);

	InitialiseCrashHandler();
	InstallInitialHooks();

	// Write launcher version to log
	spdlog::info("NorthstarLauncher version: {}", version);

	// run callbacks for any libraries that are already loaded by now
	CallAllPendingDLLLoadCallbacks();

	return true;
}
