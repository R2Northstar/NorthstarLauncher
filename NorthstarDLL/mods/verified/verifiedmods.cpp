#include "pch.h"
#include "masterserver/masterserver.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "squirrel/squirrel.h"
#include "libzip/include/zip.h"
#include "config/profile.h"
#include "verification_results.h"
#include "verifiedmodsutils.h"
#include <filesystem>
#include <spdlog/spdlog.h>
#include "openssl/evp.h"
#include <openssl/sha.h>
#include <rapidjson/fwd.h>
#include <fstream>

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

// This holds result of mod downloading and extraction.
VerificationResult modDownloadAndExtractionResult;

// This vector holds information about the currently downloaded mod:
//     1. received quantity
//     2. total quantity expected
//     3. ratio of received / expected (computed in native for PERFS!)
//     4. nature of previous data (0=download stats [in MBs], 1=extraction stats [in files count])
//
// The following information should only be used with big files (as there's no need to display this
// for small files due to extraction speed):
//     5. extracted size of current file
//     6. total size of current file
std::vector<float> currentDownloadStats(6);

// Test string used to test branch without masterserver
const char* modsTestString =
	"{"
	"\"Odd.s2space\": {"
	"\"DependencyPrefix\": \"odds-s2space\","
	"\"Versions\" : ["
	"{ \"Version\": \"0.0.4\", \"Checksum\": \"4083d0257948766d1c8eaf121395047f5de2c67be49aca937ec43d603b6c1f97\" }"
	"]}"
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
 * Fetches the list of verified mods from the master server, and store it in
 * the verifiedModsJson variable.
 * Since master server does not expose verified mods resource *yet*, this
 * uses mods stored in the modsTestString variable.
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
 * Checks if a mod is verified by controlling if its name matches a key in the
 * verified mods JSON document, and if its version is included in the JSON
 * versions list.
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
		const Value& currentModVersion = *iterator;
		const auto currentVersionText = currentModVersion["Version"].GetString();
		if (strcmp(currentVersionText, modVersion) == 0)
		{
			spdlog::info("Mod \"{}\" (version {}) is verified.", modName, modVersion);
			return true;
		}
	}

	spdlog::info("Required version {} for mod \"{}\" is not verified, and thus couldn't be downloaded.", modVersion, modName);
	return false;
}

/**
 * Tells if a mod is currently being downloaded by checking if its name is included
 * in the `modsBeingDownloaded` variable.
 **/
bool IsModBeingDownloaded(char* modName)
{
	return std::find(modsBeingDownloaded.begin(), modsBeingDownloaded.end(), modName) != modsBeingDownloaded.end();
}

/**
 * cURL method to follow download progression.
 **/
int progress_callback(
	void* ptr, curl_off_t totalDownloadSize, curl_off_t finishedDownloadSize, curl_off_t totalToUpload, curl_off_t nowUploaded)
{
	if (totalDownloadSize != 0 && finishedDownloadSize != 0)
	{
		auto currentDownloadProgress = roundf(static_cast<float>(finishedDownloadSize) / totalDownloadSize * 100);
		currentDownloadStats[0] = static_cast<float>(finishedDownloadSize);
		currentDownloadStats[1] = static_cast<float>(totalDownloadSize);
		currentDownloadStats[2] = static_cast<float>(currentDownloadProgress);
	}

	return 0;
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
	modsBeingDownloaded.push_back(modName);

	// Rebuild mod dependency string.
	const Value& entry = verifiedModsJson[modName];
	std::string dependencyString = entry["DependencyPrefix"].GetString();
	GenericArray versions = entry["Versions"].GetArray();
	dependencyString.append("-");
	dependencyString.append(modVersion);

	// Get expected checksum.
	std::string modChecksum = "";
	for (rapidjson::Value::ConstValueIterator iterator = versions.Begin(); iterator != versions.End(); iterator++)
	{
		const Value& currentModVersion = *iterator;
		const auto currentVersionText = currentModVersion["Version"].GetString();
		if (strcmp(currentVersionText, modVersion) == 0)
		{
			modChecksum = currentModVersion["Checksum"].GetString();
			break;
		}
	}
	if (modChecksum.empty())
	{
		spdlog::error("Failed to load expected archive checksum.");
		modDownloadAndExtractionResult = FAILED;
		return;
	}

	std::thread requestThread(
		[dependencyString, modName, modChecksum]()
		{
			// zip parsing variables
			zip_int64_t num_entries;
			int err = 0;
			zip_t* zip = NULL;
			struct zip_stat sb;
			std::error_code ec;
			int totalSize = 0;
			int extractedSize = 0;
			std::filesystem::path manifestPath = std::filesystem::path(GetNorthstarPrefix()) / "runtime/remote/mods/manifest.json";

			// loading game path
			std::string archiveName = (std::string)dependencyString + ".zip";
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
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
			curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);

			spdlog::info("Fetching mod {} from Thunderstore...", dependencyString);
			currentDownloadStats = {0, 0, 0, 0, 0, 0};
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
				modDownloadAndExtractionResult = MOD_FETCHING_FAILED;
				goto REQUEST_END_CLEANUP;
			}

			// Verify checksum of downloaded archive.
			if (!check_mod_archive_checksum(downloadPath, modChecksum))
			{
				spdlog::error("Archive checksum does not match verified hash.");
				modDownloadAndExtractionResult = MOD_CORRUPTED;
				goto REQUEST_END_CLEANUP;
			}
			spdlog::info("Checksum OK!");

			// Unzip mods from downloaded archive.
			zip = zip_open(downloadPath.generic_string().c_str(), ZIP_RDONLY, &err);

			if (err != ZIP_ER_OK)
			{
				spdlog::error("Opening mod archive failed: {}", zip_strerror(zip));
				modDownloadAndExtractionResult = FAILED;
				goto REQUEST_END_CLEANUP;
			}

			spdlog::info("Starting extracting files from archive.");

			// This will extract the archive contents into game folder, so we need
			// to ensure Northstar installation folder has been loaded in memory.
			InitialiseNorthstarPrefix();

			// `zip_get_name` returns a file name among files contained in the archive;
			// file names are organized as a list that has the following format:
			//     * archive.zip/icon.png
			//     * archive.zip/manifest.json
			//     * archive.zip/mods/
			//     * archive.zip/mods/firstMod/
			//     * archive.zip/mods/firstMod/mod.json
			// etc.
			num_entries = zip_get_num_entries(zip, 0);

			// Update current statistics to display files extraction progress.
			currentDownloadStats = {0, static_cast<float>(num_entries), 0, 1, 0, 0};

			// Compute total mod size to update extraction progress.
			totalSize = get_mod_archive_content_size(zip);

			// Start unzipping archive.
			for (zip_uint64_t i = 0; i < num_entries; ++i)
			{
				const char* name = zip_get_name(zip, i, 0);
				if (name == nullptr)
				{
					spdlog::error("Failed reading archive.");
					modDownloadAndExtractionResult = FAILED_READING_ARCHIVE;
					goto REQUEST_END_CLEANUP;
				}

				std::string fileName = name;

				// Only extracting files that belong to mods.
				//
				// We don't need to recreate the mods/ directory, as it should already
				// exist in a R2Northstar installation.
				//
				// A well-formatted Thunderstore archive contains files such as icon.png,
				// manifest.json and README.md; we don't want to extract those, but only
				// the content of the mods/ directory.
				if ((strcmp(name, "mods/") == 0 || fileName.substr(0, 5) != "mods/") && strcmp(name, "manifest.json") != 0)
				{
					continue;
				}

				// Use thunderstore dependency string as mod folder name.
				// (example: mods/Bombing Run/mod.json => runtime/remote/mods/Alystrasz-BombingRun-0.0.3/mod.json)
				if (strcmp(name, "manifest.json") != 0)
				{
					std::string fileNameWithoutModName = fileName.substr(fileName.find("/", 5), fileName.length());
					fileName = dependencyString + fileNameWithoutModName;
				}

				// Install mods in remote mods directory.
				fileName = "runtime/remote/mods/" + fileName;

				spdlog::info("    => {}", fileName);
				std::filesystem::path destination = std::filesystem::path(GetNorthstarPrefix()) / fileName;

				if (fileName.back() == '/')
				{
					// Create directory
					if (!std::filesystem::create_directory(destination, ec))
					{
						spdlog::error("Directory creation failed: {}", zip_strerror(zip));
						modDownloadAndExtractionResult = ec.value() == ENOSPC ? NO_DISK_SPACE_AVAILABLE : FAILED_WRITING_TO_DISK;
						goto REQUEST_END_CLEANUP;
					}

					// Extracting manifest.json to mods root directory
					if (std::count(fileName.begin(), fileName.end(), '/') == 4)
					{
						std::filesystem::copy(manifestPath, destination / "manifest.json");
						CreateModAuthorFile(
							dependencyString.substr(0, dependencyString.find('-')), destination / "thunderstore_author.txt");
					}
				}
				else
				{
					struct zip_stat sb;
					zip_stat_index(zip, i, 0, &sb);
					struct zip_file* zf = zip_fopen_index(zip, i, 0);
					std::ofstream writeStream(destination, std::ofstream::binary);

					if (!writeStream.is_open())
					{
						spdlog::error("Failed writing file to disk.");
						modDownloadAndExtractionResult = FAILED_WRITING_TO_DISK;
						zip_fclose(zf);
						goto REQUEST_END_CLEANUP;
					}

					int sum = 0;
					int len = 0;
					char buf[100];

					// Sets first statistics field to the count of extracted files, and update
					// progression percentage accordingly.
					currentDownloadStats[4] = 0;
					currentDownloadStats[5] = static_cast<float>(sb.size);

					while (sum != sb.size)
					{
						len = zip_fread(zf, buf, 100);
						writeStream.write((char*)buf, len);
						sum += len;
						extractedSize += len;
						currentDownloadStats[4] = static_cast<float>(sum);
						currentDownloadStats[2] = roundf(static_cast<float>(extractedSize) / totalSize * 100);
					}
					writeStream.close();
					zip_fclose(zf);

					// If extracted file is a mod manifest (mod top-level JSON file), we add an
					// entry to it, to mark it as "auto-downloaded".
					// (not done anymore, check https://github.com/0neGal/viper/issues/165 for
					// information on new format)
					/*
					int modNameLength = modName.length();
					if (std::count(modName.begin(), modName.end(), '/') == 2 &&
						strcmp(modName.substr(modNameLength - 8, modNameLength).c_str(), "mod.json") == 0)
					{
						if (!MarkModJson(destination))
						{
							spdlog::error("Failed to mark mod manifest");
						}
					}
					*/
				}

				// Sets first statistics field to the count of extracted files, and update
				// progression percentage regarding extracted content size.
				currentDownloadStats[0] = static_cast<float>(i);
				currentDownloadStats[2] = roundf(static_cast<float>(extractedSize) / totalSize * 100);
			}

			spdlog::info("Mod successfully extracted.");
			modDownloadAndExtractionResult = OK;

		// Called at the end of the thread, regardless if mod download and extraction went
		// successfully or not.
		REQUEST_END_CLEANUP:
			fclose(fp);
			zip_close(zip);
			try
			{
				remove(downloadPath);
			}
			catch (const std::exception& a)
			{
				spdlog::error("Error while removing downloaded archive: {}", a.what());
			}
			if (exists(manifestPath))
			{
				remove(manifestPath);
			}
			modsBeingDownloaded.erase(std::remove(std::begin(modsBeingDownloaded), std::end(modsBeingDownloaded), modName));
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

ADD_SQFUNC("string", NSGetModExtractionResult, "", "", ScriptContext::UI)
{
	g_pSquirrel<context>->pushstring(sqvm, GetVerifiedModErrorString(modDownloadAndExtractionResult));
	return SQRESULT_NOTNULL;
}

ADD_SQFUNC("array<float>", NSGetCurrentDownloadProgress, "", "", ScriptContext::UI)
{
	g_pSquirrel<context>->newarray(sqvm, 0);

	for (float value : currentDownloadStats)
	{
		g_pSquirrel<context>->pushfloat(sqvm, value);
		g_pSquirrel<context>->arrayappend(sqvm, -2);
	}

	return SQRESULT_NOTNULL;
}

ADD_SQFUNC("void", NSDownloadMod, "string modName, string modVersion", "", ScriptContext::UI)
{
	const SQChar* modName = g_pSquirrel<context>->getstring(sqvm, 1);
	const SQChar* modVersion = g_pSquirrel<context>->getstring(sqvm, 2);
	DownloadMod((char*)modName, (char*)modVersion);
	return SQRESULT_NULL;
}

ON_DLL_LOAD_CLIENT("client.dll", VerifiedMods, (CModule module))
{
	FetchVerifiedModsList();
}
