#include "moddownloader.h"
#include <rapidjson/fwd.h>

ModDownloader* g_pModDownloader;

ModDownloader::ModDownloader()
{
	spdlog::info("Mod downloaded initialized");
	modState = {};
}

size_t write_to_string(void* ptr, size_t size, size_t count, void* stream)
{
	((std::string*)stream)->append((char*)ptr, 0, size * count);
	return size * count;
}

void ModDownloader::FetchModsListFromAPI()
{
	std::thread requestThread(
		[this]()
		{
			CURLcode result;
			CURL* easyhandle;
			rapidjson::Document verifiedModsJson;
			std::string url = MODS_LIST_URL;

			curl_global_init(CURL_GLOBAL_ALL);
			easyhandle = curl_easy_init();
			std::string readBuffer;

			// Fetching mods list from GitHub repository
			curl_easy_setopt(easyhandle, CURLOPT_CUSTOMREQUEST, "GET");
			curl_easy_setopt(easyhandle, CURLOPT_TIMEOUT, 30L);
			curl_easy_setopt(easyhandle, CURLOPT_URL, url.c_str());
			curl_easy_setopt(easyhandle, CURLOPT_VERBOSE, 1L);
			curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, &readBuffer);
			curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, write_to_string);
			result = curl_easy_perform(easyhandle);

			if (result == CURLcode::CURLE_OK)
			{
				spdlog::info("Mods list successfully fetched.");
			}
			else
			{
				spdlog::error("Fetching mods list failed: {}", curl_easy_strerror(result));
				goto REQUEST_END_CLEANUP;
			}

			// Load mods list into local state
			spdlog::info("Loading mods configuration...");
			verifiedModsJson.Parse(readBuffer);
			for (auto i = verifiedModsJson.MemberBegin(); i != verifiedModsJson.MemberEnd(); ++i)
			{
				std::string name = i->name.GetString();
				std::string dependency = i->value["DependencyPrefix"].GetString();

				std::unordered_map<std::string, VerifiedModVersion> modVersions;
				rapidjson::Value& versions = i->value["Versions"];
				assert(versions.IsArray());
				for (rapidjson::Value::ConstValueIterator itr = versions.Begin(); itr != versions.End(); ++itr)
				{
					const rapidjson::Value& attribute = *itr;
					assert(attribute.IsObject());
					std::string version = attribute["Version"].GetString();
					std::string checksum = attribute["Checksum"].GetString();
					modVersions.insert({version, {.checksum = checksum}});
				}

				VerifiedModDetails modConfig = {.dependencyPrefix = dependency, .versions = modVersions};
				verifiedMods.insert({name, modConfig});
				spdlog::info("==> Loaded configuration for mod \"" + name + "\"");
			}

			spdlog::info("Done loading verified mods list.");

		REQUEST_END_CLEANUP:
			curl_easy_cleanup(easyhandle);
		});
	requestThread.detach();
}

size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream)
{
	size_t written;
	written = fwrite(ptr, size, nmemb, stream);
	return written;
}

fs::path ModDownloader::FetchModFromDistantStore(std::string modName, std::string modVersion)
{
	// Build archive distant URI
	std::string archiveName = std::format("{}-{}.zip", verifiedMods[modName].dependencyPrefix, modVersion);
	std::string url = STORE_URL + archiveName;
	spdlog::info(std::format("Fetching mod archive from {}", url));

	// Download destination
	std::filesystem::path downloadPath = std::filesystem::temp_directory_path() / archiveName;
	spdlog::info(std::format("Downloading archive to {}", downloadPath.generic_string()));

	// Download the actual archive
	std::thread requestThread(
		[this, url, downloadPath]()
		{
			FILE* fp = fopen(downloadPath.generic_string().c_str(), "wb");
			CURLcode result;
			CURL* easyhandle;
			easyhandle = curl_easy_init();

			curl_easy_setopt(easyhandle, CURLOPT_TIMEOUT, 30L);
			curl_easy_setopt(easyhandle, CURLOPT_URL, url.c_str());
			curl_easy_setopt(easyhandle, CURLOPT_VERBOSE, 1L);
			curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, fp);
			curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, write_data);
			result = curl_easy_perform(easyhandle);

			if (result == CURLcode::CURLE_OK)
			{
				spdlog::info("Mod archive successfully fetched.");
			}
			else
			{
				spdlog::error("Fetching mod archive failed: {}", curl_easy_strerror(result));
				goto REQUEST_END_CLEANUP;
			}

		REQUEST_END_CLEANUP:
			curl_easy_cleanup(easyhandle);
			fclose(fp);
		});

	requestThread.join();
	return downloadPath;
}

bool ModDownloader::IsModLegit(fs::path modPath, std::string expectedChecksum)
{
	// TODO implement
	return true;
}

bool ModDownloader::IsModAuthorized(std::string modName, std::string modVersion)
{
	if (!verifiedMods.contains(modName))
	{
		return false;
	}

	std::unordered_map<std::string, VerifiedModVersion> versions = verifiedMods[modName].versions;
	return versions.count(modVersion) != 0;
}

void ModDownloader::DownloadMod(std::string modName, std::string modVersion)
{
	// Check if mod can be auto-downloaded
	if (!IsModAuthorized(modName, modVersion))
	{
		spdlog::warn("Tried to download a mod that is not verified, aborting.");
		return;
	}

	std::thread requestThread(
		[this, modName, modVersion]()
		{
			// Download mod archive
			std::string expectedHash = verifiedMods[modName].versions[modVersion].checksum;
			fs::path archiveLocation = FetchModFromDistantStore(modName, modVersion);
			if (!IsModLegit(archiveLocation, expectedHash))
			{
				spdlog::warn("Archive hash does not match expected checksum, aborting.");
				return;
			}

			// TODO extract mod archive

		REQUEST_END_CLEANUP:
			spdlog::info("ok");
		});

	requestThread.detach();
}

void ConCommand_fetch_verified_mods(const CCommand& args)
{
	g_pModDownloader->FetchModsListFromAPI();
}

void ConCommand_is_mod_verified(const CCommand& args)
{
	if (args.ArgC() < 3)
	{
		return;
	}

	// Split arguments string by whitespaces (https://stackoverflow.com/a/5208977)
	std::string buf;
	std::stringstream ss(args.ArgS());
	std::vector<std::string> tokens;
	while (ss >> buf)
		tokens.push_back(buf);

	std::string modName = tokens[0];
	std::string modVersion = tokens[1];
	bool result = g_pModDownloader->IsModAuthorized(modName, modVersion);
	std::string msg = std::format("Mod \"{}\" (version {}) is verified: {}", modName, modVersion, result);
	spdlog::info(msg);
}

void ConCommand_download_mod(const CCommand& args)
{
	if (args.ArgC() < 3)
	{
		return;
	}

	// Split arguments string by whitespaces (https://stackoverflow.com/a/5208977)
	std::string buf;
	std::stringstream ss(args.ArgS());
	std::vector<std::string> tokens;
	while (ss >> buf)
		tokens.push_back(buf);

	std::string modName = tokens[0];
	std::string modVersion = tokens[1];
	g_pModDownloader->DownloadMod(modName, modVersion);
}

ON_DLL_LOAD_RELIESON("engine.dll", ModDownloader, (ConCommand), (CModule module))
{
	g_pModDownloader = new ModDownloader();
	RegisterConCommand(
		"fetch_verified_mods", ConCommand_fetch_verified_mods, "fetches verified mods list from GitHub repository", FCVAR_NONE);
	RegisterConCommand("is_mod_verified", ConCommand_is_mod_verified, "checks if a mod is included in verified mods list", FCVAR_NONE);
	RegisterConCommand("download_mod", ConCommand_download_mod, "downloads a mod from remote store", FCVAR_NONE);
}
