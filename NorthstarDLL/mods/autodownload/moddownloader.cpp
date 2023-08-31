#include "moddownloader.h"
#include <rapidjson/fwd.h>
#include <mz_strm_mem.h>
#include <mz.h>
#include <mz_strm.h>
#include <mz_zip.h>
#include <mz_compat.h>

ModDownloader* g_pModDownloader;

ModDownloader::ModDownloader()
{
	spdlog::info("Mod downloaded initialized");
	modState = {};
}

size_t writeToString(void* ptr, size_t size, size_t count, void* stream)
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
			curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, writeToString);
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

size_t writeData(void* ptr, size_t size, size_t nmemb, FILE* stream)
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
			curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, writeData);
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

void ModDownloader::ExtractMod(fs::path modPath)
{
	unzFile file;

	file = unzOpen(modPath.generic_string().c_str());
	if (file == NULL)
	{
		spdlog::error("Cannot open archive located at {}.", modPath.generic_string());
		return;
	}

	unz_global_info64 gi;
	int status;
	status = unzGetGlobalInfo64(file, &gi);
	if (file != UNZ_OK)
	{
		spdlog::error("Failed getting information from archive (error code: {})", status);
	}

	// Mod directory name (removing the ".zip" fom the archive name)
	std::string name = modPath.filename().string();
	name = name.substr(0, name.length() - 4);
	fs::path modDirectory = GetRemoteModFolderPath() / name;

	for (int i = 0; i < gi.number_entry; i++)
	{
		char zipFilename[256];
		unz_file_info64 fileInfo;
		status = unzGetCurrentFileInfo64(file, &fileInfo, zipFilename, sizeof(zipFilename), NULL, 0, NULL, 0);

		// Extract file
		{
			std::error_code ec;
			fs::path fileDestination = modDirectory / zipFilename;
			spdlog::info("=> {}", fileDestination.generic_string());

			// Create parent directory if needed
			if (!std::filesystem::exists(fileDestination.parent_path()))
			{
				spdlog::info("Parent directory does not exist, creating it.", fileDestination.generic_string());
				if (!std::filesystem::create_directories(fileDestination.parent_path(), ec) && ec.value() != 0)
				{
					spdlog::error("Parent directory ({}) creation failed.", fileDestination.parent_path().generic_string());
					return;
				}
			}

			// If current file is a directory, create directory...
			if (fileDestination.generic_string().back() == '/')
			{
				// Create directory
				if (!std::filesystem::create_directory(fileDestination, ec) && ec.value() != 0)
				{
					spdlog::error("Directory creation failed: {}", ec.message());
					return;
				}
			}

			// ...else create file
			else
			{
				// Ensure file is in zip archive
				if (unzLocateFile(file, zipFilename, 0) != UNZ_OK)
				{
					spdlog::error("File \"{}\" was not found in archive.", zipFilename);
					return;
				}

				// Create file
				int bufferSize;
				void* buffer;
				int err = UNZ_OK;
				FILE* fout = NULL;

				// Open zip file to prepare its extraction
				status = unzOpenCurrentFile(file);
				if (status != UNZ_OK)
				{
					spdlog::error("Could not open file {} from archive.", zipFilename);
				}

				// Create destination file
				fout = fopen(fileDestination.generic_string().c_str(), "wb");
				if (fout == NULL)
				{
					spdlog::error("Failed creating destination file.");
					return;
				}

				// Allocate memory for buffer
				bufferSize = 8192;
				buffer = (void*)malloc(bufferSize);
				if (buffer == NULL)
				{
					spdlog::error("Error while allocating memory.");
					return;
				}

				// Extract file to destination
				do
				{
					err = unzReadCurrentFile(file, buffer, bufferSize);
					if (err < 0)
					{
						spdlog::error("error {} with zipfile in unzReadCurrentFile", err);
						break;
					}
					if (err > 0)
					{
						if (fwrite(buffer, (unsigned)err, 1, fout) != 1)
						{
							spdlog::error("error in writing extracted file\n");
							err = UNZ_ERRNO;
							break;
						}
					}
				} while (err > 0);

				if (err != UNZ_OK)
				{
					spdlog::error("An error occurred during file extraction (code: {})", err);
				}
				err = unzCloseCurrentFile(file);
				if (err != UNZ_OK)
				{
					spdlog::error("error {} with zipfile in unzCloseCurrentFile", err);
				}

				// Cleanup
				if (fout)
					fclose(fout);
			}
		}

		// Go to next file
		if ((i + 1) < gi.number_entry)
		{
			status = unzGoToNextFile(file);
			if (status != UNZ_OK)
			{
				spdlog::error("Error while browsing archive files (error code: {}).", status);
				break;
			}
		}
	}
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

			// Extract downloaded mod archive
			ExtractMod(archiveLocation);

		REQUEST_END_CLEANUP:
			spdlog::info("Done downloading {}.", modName);
		});

	requestThread.detach();
}

void ConCommandFetchVerifiedMods(const CCommand& args)
{
	g_pModDownloader->FetchModsListFromAPI();
}

void ConCommandDownloadMod(const CCommand& args)
{
	if (args.ArgC() < 3)
	{
		return;
	}

	// Split arguments string by whitespaces (https://stackoverflow.com/a/5208977)
	std::string buffer;
	std::stringstream ss(args.ArgS());
	std::vector<std::string> tokens;
	while (ss >> buffer)
		tokens.push_back(buffer);

	std::string modName = tokens[0];
	std::string modVersion = tokens[1];
	g_pModDownloader->DownloadMod(modName, modVersion);
}

ON_DLL_LOAD_RELIESON("engine.dll", ModDownloader, (ConCommand), (CModule module))
{
	g_pModDownloader = new ModDownloader();
	RegisterConCommand("fetch_verified_mods", ConCommandFetchVerifiedMods, "fetches verified mods list from GitHub repository", FCVAR_NONE);
	RegisterConCommand("download_mod", ConCommandDownloadMod, "downloads a mod from remote store", FCVAR_NONE);
}
