#include "moddownloader.h"

void ModDownloader::FetchModsListFromAPI() {
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
