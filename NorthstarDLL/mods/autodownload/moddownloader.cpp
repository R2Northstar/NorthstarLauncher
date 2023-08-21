#include "moddownloader.h"

ModDownloader* g_pModDownloader;

ModDownloader::ModDownloader() {}

void ModDownloader::FetchModsListFromAPI()
{
	const char* url = MODS_LIST_URL;

	std::thread requestThread(
		[url]()
		{
		curl_global_init(CURL_GLOBAL_ALL);
		CURL* easyhandle = curl_easy_init();
		std::string readBuffer;

		curl_easy_setopt(easyhandle, CURLOPT_URL, url);
		curl_easy_setopt(easyhandle, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, &readBuffer);

		curl_easy_perform(easyhandle);
		std::cout << readBuffer << std::endl;
	});
}

void ConCommand_fetch_verified_mods(const CCommand& args)
{
	g_pModDownloader->FetchModsListFromAPI();
}

ON_DLL_LOAD_RELIESON("engine.dll", ModDownloader, (ConCommand), (CModule module))
{
	g_pModDownloader = new ModDownloader();
	RegisterConCommand("fetch_verified_mods", ConCommand_fetch_verified_mods, "fetches verified mods list from GitHub repository", FCVAR_NONE);
}
