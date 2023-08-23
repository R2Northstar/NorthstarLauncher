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

				std::vector<VerifiedModVersion> modVersions;
				rapidjson::Value& versions = i->value["Versions"];
				assert(versions.IsArray());
				for (rapidjson::Value::ConstValueIterator itr = versions.Begin(); itr != versions.End(); ++itr)
				{
					const rapidjson::Value& attribute = *itr;
					assert(attribute.IsObject());
					std::string version = attribute["Version"].GetString();
					std::string checksum = attribute["Checksum"].GetString();
					modVersions.push_back({.version = version, .checksum = checksum});
				}

				VerifiedModDetails modConfig = {.dependencyPrefix = (char*)dependency.c_str(), .versions = modVersions};
				verifiedMods.insert({name, modConfig});
				spdlog::info("==> Loaded configuration for mod \"" + name + "\"");
			}

			spdlog::info("Done loading verified mods list.");

		REQUEST_END_CLEANUP:
			curl_easy_cleanup(easyhandle);
		});
	requestThread.detach();
}

fs::path ModDownloader::FetchModFromDistantStore(char* modName, char* modVersion)
{
	return fs::path();
}

bool ModDownloader::IsModLegit(fs::path modPath, char* expectedChecksum)
{
	return false;
}

bool ModDownloader::IsModAuthorized(char* modName, char* modVersion)
{
	if (!verifiedMods.contains(modName))
	{
		return false;
	}

	std::vector<VerifiedModVersion> versions = verifiedMods[modName].versions;
	std::vector<VerifiedModVersion> matchingVersions;
	std::copy_if(
		versions.begin(),
		versions.end(),
		std::back_inserter(matchingVersions),
		[modVersion](VerifiedModVersion v) { return strcmp(modVersion, v.version.c_str()) == 0; });

	return matchingVersions.size() != 0;
}

void ModDownloader::DownloadMod(char* modName, char* modVersion)
{
	// Check if mod can be auto-downloaded
	if (!IsModAuthorized(modName, modVersion))
	{
		spdlog::warn("Tried to download a mod that is not verified, aborting.");
		return;
	}

	// Download mod archive
	std::string expectedHash = "TODO";
	fs::path archiveLocation = FetchModFromDistantStore(modName, modVersion);
	if (!IsModLegit(archiveLocation, (char*)expectedHash.c_str()))
	{
		spdlog::warn("Archive hash does not match expected checksum, aborting.");
		return;
	}

	// TODO extract mod archive
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

	char* modName = (char*)tokens[0].c_str();
	char* modVersion = (char*)tokens[1].c_str();
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

	char* modName = (char*)tokens[0].c_str();
	char* modVersion = (char*)tokens[1].c_str();
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
