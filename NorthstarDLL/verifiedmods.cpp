#include "pch.h"
#include "masterserver.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "squirrel.h"
#include "verifiedmods.h"

using namespace rapidjson;



/*
 ██████╗ ██╗      ██████╗ ██████╗  █████╗ ██╗         ██╗   ██╗ █████╗ ██████╗ ██╗ █████╗ ██████╗ ██╗     ███████╗███████╗
██╔════╝ ██║     ██╔═══██╗██╔══██╗██╔══██╗██║         ██║   ██║██╔══██╗██╔══██╗██║██╔══██╗██╔══██╗██║     ██╔════╝██╔════╝
██║  ███╗██║     ██║   ██║██████╔╝███████║██║         ██║   ██║███████║██████╔╝██║███████║██████╔╝██║     █████╗  ███████╗
██║   ██║██║     ██║   ██║██╔══██╗██╔══██║██║         ╚██╗ ██╔╝██╔══██║██╔══██╗██║██╔══██║██╔══██╗██║     ██╔══╝  ╚════██║
╚██████╔╝███████╗╚██████╔╝██████╔╝██║  ██║███████╗     ╚████╔╝ ██║  ██║██║  ██║██║██║  ██║██████╔╝███████╗███████╗███████║
 ╚═════╝ ╚══════╝ ╚═════╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝      ╚═══╝  ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝╚═╝  ╚═╝╚═════╝ ╚══════╝╚══════╝╚══════╝
*/


// This JSON contains all verified mods.
Document verifiedModsJson;

// This list holds the names of all mods that are currently being downloaded.
std::vector<std::string> modsBeingDownloaded {};

// Test string used to test branch without masterserver
const char* modsTestString =
	"{"
	"\"Dinorush's LTS Rebalance\" : {\"DependencyPrefix\" : \"Dinorush-LTSRebalance\", \"Versions\" : []}, "
	"\"Dinorush.Brute4\" : {\"DependencyPrefix\" : \"Dinorush-DinorushBrute4\", \"Versions\" : [  \"1.5\", \"1.6.0\" ]}, "
	"\"Mod Settings\" : {\"DependencyPrefix\" : \"EladNLG-ModSettings\", \"Versions\" : [ \"1.0.0\", \"1.1.0\" ]}, "
	"\"Moblin.Archon\" : {\"DependencyPrefix\" : \"GalacticMoblin-MoblinArchon\", \"Versions\" : [ \"1.3.0\", \"1.3.1\" ]},"
	"\"Fifty.mp_frostbite\" : {\"DependencyPrefix\" : \"Fifty-Frostbite\", \"Versions\" : [ \"0.0.1\" ]}"
	"}";



/*
███╗   ███╗███████╗████████╗██╗  ██╗ ██████╗ ██████╗ ███████╗
████╗ ████║██╔════╝╚══██╔══╝██║  ██║██╔═══██╗██╔══██╗██╔════╝
██╔████╔██║█████╗     ██║   ███████║██║   ██║██║  ██║███████╗
██║╚██╔╝██║██╔══╝     ██║   ██╔══██║██║   ██║██║  ██║╚════██║
██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝██████╔╝███████║
╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚══════╝
*/


/**
 * Fetches the list of verified mods from the master server, and store it in the verifiedModsJson variable.
 * Since master server does not expose verified mods resource *yet*, this uses mods stored in the modsTestString variable.
 **/
void FetchVerifiedModsList()
{
	spdlog::info("Requesting verified mods list from {}", Cvar_ns_masterserver_hostname->GetString());

	std::thread requestThread(
		[]()
		{
			// TODO fetch list from masterserver
			verifiedModsJson.Parse(modsTestString);
			goto REQUEST_END_CLEANUP;

		REQUEST_END_CLEANUP:
			spdlog::info("Verified mods list successfully fetched.");
		});
	requestThread.detach();
}


/**
 * Checks if a mod is verified by controlling if its name matches a key in the verified mods JSON
 * document, and if its version is included in the JSON versions list.
 **/
bool IsModVerified(char* modName, char* modVersion)
{
	// 1. Mod is not verified if its name isn't a `verifiedModsJson` key.
	if (!verifiedModsJson.HasMember(modName))
	{
		spdlog::info("Mod \"{}\" is not verified, and thus couldn't be downloaded.", modName);
		return false;
	}

	// 2. Check if mod version has been validated.
	const Value& entry = verifiedModsJson[modName];
	GenericArray versions = entry["Versions"].GetArray();

	spdlog::info("There's an entry for mod \"{}\", now checking version...", modName);

	for (rapidjson::Value::ConstValueIterator iterator = versions.Begin(); iterator != versions.End(); iterator++)
	{
		const rapidjson::Value& version = *iterator;
		if (strcmp(version.GetString(), modVersion) == 0)
		{
			spdlog::info("Mod \"{}\" (version {}) is verified.", modName, modVersion);
			return true;
		}
	}

	spdlog::info("Required version {} for mod \"{}\" is not verified, and thus couldn't be downloaded.", modVersion, modName);
	return false;
}


/**
 * Tells if a mod is currently being downloaded by checking if its name is included in the `modsBeingDownloaded` variable.
 **/
bool IsModBeingDownloaded(char* modName)
{
	return std::find(modsBeingDownloaded.begin(), modsBeingDownloaded.end(), modName) != modsBeingDownloaded.end();
}


/**
 * cURL method to write data to disk.
 **/
size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream)
{
	size_t written;
	written = fwrite(ptr, size, nmemb, stream);
	return written;
}

/**
 * Downloads a given mod from Thunderstore API to local game folder.
 * This is done following these steps:
 *     * Recreating mod dependency string from verified mods information;
 *     * Fetching mod .zip archive from Thunderstore API to local temporary storage;
 *     * Extracting archive content into game folder;
 *     * Cleaning.
 **/
void DownloadMod(char* modName, char* modVersion)
{
	if (!IsModVerified(modName, modVersion))
		return;

	modsBeingDownloaded.push_back( modName );

	// Rebuild mod dependency string.
	const Value& entry = verifiedModsJson[modName];
	std::string dependencyString = entry["DependencyPrefix"].GetString();
	GenericArray versions = entry["Versions"].GetArray();
	dependencyString.append("-");
	dependencyString.append(modVersion);

	std::thread requestThread(
		[dependencyString, modName]()
		{
			// loading game path
			std::filesystem::path downloadPath = std::filesystem::temp_directory_path() / ((std::string)dependencyString + ".zip");
			
			CURL* curl = curl_easy_init();
			FILE* fp = fopen(downloadPath.generic_string().c_str(), "wb");
			CURLcode result;

			std::string url = "https://gcdn.thunderstore.io/live/repository/packages/" + (std::string)dependencyString + ".zip";
			spdlog::info("Downloading mod:");
			spdlog::info("    => from {}", url);
			spdlog::info("    => to {}", downloadPath.generic_string());

			curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		
			spdlog::info("Fetching mod {} from Thunderstore...", dependencyString);
			result = curl_easy_perform(curl);
			curl_easy_cleanup(curl);

			if (result == CURLcode::CURLE_OK)
			{
				spdlog::info("Ok");
			}
			else
			{
				spdlog::info("Fetching mod failed: {}", curl_easy_strerror(result));
				return;
			}

			// TODO unzip folder
			// TODO move mod to mods/ folder
			// TODO remove temporary folder

		REQUEST_END_CLEANUP:
			fclose(fp);
			modsBeingDownloaded.erase( std::remove(std::begin(modsBeingDownloaded), std::end(modsBeingDownloaded), modName) );
		});
	requestThread.detach();
}



/*
███████╗ ██████╗ ██╗   ██╗██╗██████╗ ██████╗ ███████╗██╗      ███████╗██╗  ██╗██████╗  ██████╗ ███████╗███████╗██████╗
██╔════╝██╔═══██╗██║   ██║██║██╔══██╗██╔══██╗██╔════╝██║      ██╔════╝╚██╗██╔╝██╔══██╗██╔═══██╗██╔════╝██╔════╝██╔══██╗
███████╗██║   ██║██║   ██║██║██████╔╝██████╔╝█████╗  ██║█████╗█████╗   ╚███╔╝ ██████╔╝██║   ██║███████╗█████╗  ██║  ██║
╚════██║██║▄▄ ██║██║   ██║██║██╔══██╗██╔══██╗██╔══╝  ██║╚════╝██╔══╝   ██╔██╗ ██╔═══╝ ██║   ██║╚════██║██╔══╝  ██║  ██║
███████║╚██████╔╝╚██████╔╝██║██║  ██║██║  ██║███████╗███████╗ ███████╗██╔╝ ██╗██║     ╚██████╔╝███████║███████╗██████╔╝
╚══════╝ ╚══▀▀═╝  ╚═════╝ ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝╚══════╝ ╚══════╝╚═╝  ╚═╝╚═╝      ╚═════╝ ╚══════╝╚══════╝╚═════╝

██╗    ██╗██████╗  █████╗ ██████╗ ██████╗ ███████╗██████╗     ███╗   ███╗███████╗████████╗██╗  ██╗ ██████╗ ██████╗ ███████╗
██║    ██║██╔══██╗██╔══██╗██╔══██╗██╔══██╗██╔════╝██╔══██╗    ████╗ ████║██╔════╝╚══██╔══╝██║  ██║██╔═══██╗██╔══██╗██╔════╝
██║ █╗ ██║██████╔╝███████║██████╔╝██████╔╝█████╗  ██████╔╝    ██╔████╔██║█████╗     ██║   ███████║██║   ██║██║  ██║███████╗
██║███╗██║██╔══██╗██╔══██║██╔═══╝ ██╔═══╝ ██╔══╝  ██╔══██╗    ██║╚██╔╝██║██╔══╝     ██║   ██╔══██║██║   ██║██║  ██║╚════██║
╚███╔███╔╝██║  ██║██║  ██║██║     ██║     ███████╗██║  ██║    ██║ ╚═╝ ██║███████╗   ██║   ██║  ██║╚██████╔╝██████╔╝███████║
 ╚══╝╚══╝ ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝     ╚═╝     ╚══════╝╚═╝  ╚═╝    ╚═╝     ╚═╝╚══════╝   ╚═╝   ╚═╝  ╚═╝ ╚═════╝ ╚═════╝ ╚══════╝
*/


ADD_SQFUNC("string", FetchVerifiedModsList, "", "", ScriptContext::UI)
{
	FetchVerifiedModsList();
	return SQRESULT_NULL;
}

ADD_SQFUNC("bool", IsModVerified, "string modName, string modVersion", "", ScriptContext::UI)
{
	const SQChar* modName = g_pSquirrel<context>->getstring(sqvm, 1);
	const SQChar* modVersion = g_pSquirrel<context>->getstring(sqvm, 2);

	bool result = IsModVerified((char*)modName, (char*)modVersion);
	g_pSquirrel<context>->pushbool(sqvm, result);

	return SQRESULT_NOTNULL;
}

ADD_SQFUNC("bool", IsModBeingDownloaded, "string modName", "", ScriptContext::UI)
{
	const SQChar* modName = g_pSquirrel<context>->getstring(sqvm, 1);

	bool result = IsModBeingDownloaded((char*)modName);
	g_pSquirrel<context>->pushbool(sqvm, result);

	return SQRESULT_NOTNULL;
}

ADD_SQFUNC("void", DownloadMod, "string modName, string modVersion", "", ScriptContext::UI)
{
	const SQChar* modName = g_pSquirrel<context>->getstring(sqvm, 1);
	const SQChar* modVersion = g_pSquirrel<context>->getstring(sqvm, 2);
	DownloadMod((char*)modName, (char*)modVersion);
	return SQRESULT_NULL;
}
