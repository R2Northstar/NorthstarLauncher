#include "pch.h"
#include "masterserver.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "squirrel.h"
#include "verifiedmods.h"
#include "libzip/include/zip.h"
#include <fstream>
#include "nsprefix.h"

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
 *
 * Before calling this, you MUST ensure mod is verified by invoking the
 * IsModVerified method.
 **/
void DownloadMod(char* modName, char* modVersion)
{
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
			std::string archiveName = (std::string)dependencyString + ".zip";
			// loading game path
			std::filesystem::path downloadPath = std::filesystem::temp_directory_path() / archiveName;
			
			CURL* curl = curl_easy_init();
			FILE* fp = fopen(downloadPath.generic_string().c_str(), "wb");
			CURLcode result;

			std::string url = "https://gcdn.thunderstore.io/live/repository/packages/" + archiveName;
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
				spdlog::info("Mod successfully fetched.");
				fclose(fp);
			}
			else
			{
				spdlog::info("Fetching mod failed: {}", curl_easy_strerror(result));
				return;
			}

			// Unzip mods from downloaded archive.
			int err = 0;
			zip_t* zip = zip_open(downloadPath.generic_string().c_str(), ZIP_RDONLY, &err);

			if (err != ZIP_ER_OK)
			{
				spdlog::error("Opening mod archive failed: {}", zip_strerror(zip));
				return;
			}

			zip_file* p_file = zip_fopen(zip, "mods/", 0);
			if (p_file == NULL)
			{
				spdlog::error("Reading mod archive failed: {}", zip_strerror(zip));
				return;
			}

			spdlog::info("Starting extracting files from archive.");

			InitialiseNorthstarPrefix();

			zip_int64_t num_entries = zip_get_num_entries(zip, 0);
			for (zip_uint64_t i = 0; i < num_entries; ++i)
			{
				const char* name = zip_get_name(zip, i, 0);
				if (name == nullptr)
				{
					spdlog::error("Failed reading archive.");
					return;
				}

				// Only extracting files that belong to mods.
				std::string modName = name;
				if (strcmp(name, "mods/") == 0 || modName.substr(0, 5) != "mods/")
				{
					continue;
				}

				spdlog::info("    => {}", name);
				std::filesystem::path destination = std::filesystem::path(GetNorthstarPrefix()) / name;

				if (modName.back() == '/')
				{
					std::filesystem::create_directory(destination);
				}
				else
				{
					struct zip_stat sb;
					zip_stat_index(zip, i, 0, &sb);
					struct zip_file* zf = zip_fopen_index(zip, i, 0);
					std::ofstream writeStream(destination, std::ofstream::binary);

					int sum = 0;
					int len = 0;
					char buf[100];

					while (sum != sb.size)
					{
						len = zip_fread(zf, buf, 100);
						writeStream.write((char*)buf, len);
						sum += len;
					}
					writeStream.close();
				}
			}
		
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


ADD_SQFUNC("string", NSFetchVerifiedModsList, "", "", ScriptContext::UI)
{
	FetchVerifiedModsList();
	return SQRESULT_NULL;
}

ADD_SQFUNC("bool", NSIsModVerified, "string modName, string modVersion", "", ScriptContext::UI)
{
	const SQChar* modName = g_pSquirrel<context>->getstring(sqvm, 1);
	const SQChar* modVersion = g_pSquirrel<context>->getstring(sqvm, 2);

	bool result = IsModVerified((char*)modName, (char*)modVersion);
	g_pSquirrel<context>->pushbool(sqvm, result);

	return SQRESULT_NOTNULL;
}

ADD_SQFUNC("bool", NSIsModBeingDownloaded, "string modName", "", ScriptContext::UI)
{
	const SQChar* modName = g_pSquirrel<context>->getstring(sqvm, 1);

	bool result = IsModBeingDownloaded((char*)modName);
	g_pSquirrel<context>->pushbool(sqvm, result);

	return SQRESULT_NOTNULL;
}

ADD_SQFUNC("void", NSDownloadMod, "string modName, string modVersion", "", ScriptContext::UI)
{
	const SQChar* modName = g_pSquirrel<context>->getstring(sqvm, 1);
	const SQChar* modVersion = g_pSquirrel<context>->getstring(sqvm, 2);
	DownloadMod((char*)modName, (char*)modVersion);
	return SQRESULT_NULL;
}
