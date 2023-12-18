#include "moddownloader.h"
#include <rapidjson/fwd.h>
#include <mz_strm_mem.h>
#include <mz.h>
#include <mz_strm.h>
#include <mz_zip.h>
#include <mz_compat.h>
#include <thread>
#include <future>
#include <bcrypt.h>
#include <winternl.h>
#include <fstream>

ModDownloader* g_pModDownloader;

ModDownloader::ModDownloader()
{
	DevMsg(eLog::NS, "Mod downloader initialized\n");

	// Initialise mods list URI
	char* clachar = strstr(GetCommandLineA(), CUSTOM_MODS_URL_FLAG);
	if (clachar)
	{
		std::string url;
		int iFlagStringLength = strlen(CUSTOM_MODS_URL_FLAG);
		std::string cla = std::string(clachar);
		if (strncmp(cla.substr(iFlagStringLength, 1).c_str(), "\"", 1))
		{
			int space = cla.find(" ");
			url = cla.substr(iFlagStringLength, space - iFlagStringLength);
		}
		else
		{
			std::string quote = "\"";
			int quote1 = cla.find(quote);
			int quote2 = (cla.substr(quote1 + 1)).find(quote);
			url = cla.substr(quote1 + 1, quote2);
		}
		DevMsg(eLog::NS, "Found custom verified mods URL in command line argument: %s\n", url.c_str());
		modsListUrl = strdup(url.c_str());
	}
	else
	{
		DevMsg(eLog::NS, "Custom verified mods URL not found in command line arguments, using default URL.\n");
		modsListUrl = strdup(DEFAULT_MODS_LIST_URL);
	}
}

size_t WriteToString(void* ptr, size_t size, size_t count, void* stream)
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
			std::string url = modsListUrl;

			curl_global_init(CURL_GLOBAL_ALL);
			easyhandle = curl_easy_init();
			std::string readBuffer;

			// Fetching mods list from GitHub repository
			curl_easy_setopt(easyhandle, CURLOPT_CUSTOMREQUEST, "GET");
			curl_easy_setopt(easyhandle, CURLOPT_TIMEOUT, 30L);
			curl_easy_setopt(easyhandle, CURLOPT_URL, url.c_str());
			curl_easy_setopt(easyhandle, CURLOPT_FAILONERROR, 1L);
			curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, &readBuffer);
			curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, WriteToString);
			result = curl_easy_perform(easyhandle);

			if (result == CURLcode::CURLE_OK)
			{
				DevMsg(eLog::NS, "Mods list successfully fetched.\n");
			}
			else
			{
				Error(eLog::NS, NO_ERROR, "Fetching mods list failed: %s\n", curl_easy_strerror(result));
				goto REQUEST_END_CLEANUP;
			}

			// Load mods list into local state
			DevMsg(eLog::NS, "Loading mods configuration...\n");
			verifiedModsJson.Parse(readBuffer);
			for (auto i = verifiedModsJson.MemberBegin(); i != verifiedModsJson.MemberEnd(); ++i)
			{
				std::string name = i->name.GetString();
				std::string dependency = i->value["DependencyPrefix"].GetString();

				std::unordered_map<std::string, VerifiedModVersion> modVersions;
				rapidjson::Value& versions = i->value["Versions"];
				assert(versions.IsArray());
				for (auto& attribute : versions.GetArray())
				{
					assert(attribute.IsObject());
					std::string version = attribute["Version"].GetString();
					std::string checksum = attribute["Checksum"].GetString();
					modVersions.insert({version, {.checksum = checksum}});
				}

				VerifiedModDetails modConfig = {.dependencyPrefix = dependency, .versions = modVersions};
				verifiedMods.insert({name, modConfig});
				DevMsg(eLog::NS, "==> Loaded configuration for mod \"%s\n", name.c_str());
			}

			DevMsg(eLog::NS, "Done loading verified mods list.\n");

		REQUEST_END_CLEANUP:
			curl_easy_cleanup(easyhandle);
		});
	requestThread.detach();
}

size_t WriteData(void* ptr, size_t size, size_t nmemb, FILE* stream)
{
	size_t written;
	written = fwrite(ptr, size, nmemb, stream);
	return written;
}

void FetchModSync(std::promise<std::optional<fs::path>>&& p, std::string_view url, fs::path downloadPath)
{
	bool failed = false;
	FILE* fp = fopen(downloadPath.generic_string().c_str(), "wb");
	CURLcode result;
	CURL* easyhandle;
	easyhandle = curl_easy_init();

	curl_easy_setopt(easyhandle, CURLOPT_TIMEOUT, 30L);
	curl_easy_setopt(easyhandle, CURLOPT_URL, url.data());
	curl_easy_setopt(easyhandle, CURLOPT_FAILONERROR, 1L);
	curl_easy_setopt(easyhandle, CURLOPT_WRITEDATA, fp);
	curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, WriteData);
	result = curl_easy_perform(easyhandle);

	if (result == CURLcode::CURLE_OK)
	{
		DevMsg(eLog::NS, "Mod archive successfully fetched.\n");
		goto REQUEST_END_CLEANUP;
	}
	else
	{
		Error(eLog::NS, NO_ERROR, "Fetching mod archive failed: %s\n", curl_easy_strerror(result));
		failed = true;
		goto REQUEST_END_CLEANUP;
	}

REQUEST_END_CLEANUP:
	curl_easy_cleanup(easyhandle);
	fclose(fp);
	p.set_value(failed ? std::optional<fs::path>() : std::optional<fs::path>(downloadPath));
}

std::optional<fs::path> ModDownloader::FetchModFromDistantStore(std::string_view modName, std::string_view modVersion)
{
	// Retrieve mod prefix from local mods list, or use mod name as mod prefix if bypass flag is set
	std::string modPrefix = strstr(GetCommandLineA(), VERIFICATION_FLAG) ? modName.data() : verifiedMods[modName.data()].dependencyPrefix;
	// Build archive distant URI
	std::string archiveName = std::format("{}-{}.zip", modPrefix, modVersion.data());
	std::string url = STORE_URL + archiveName;
	DevMsg(eLog::NS, "Fetching mod archive from %s\n", url.c_str());

	// Download destination
	std::filesystem::path downloadPath = std::filesystem::temp_directory_path() / archiveName;
	DevMsg(eLog::NS, "Downloading archive to %s\n", downloadPath.c_str());

	// Download the actual archive
	std::promise<std::optional<fs::path>> promise;
	auto f = promise.get_future();
	std::thread t(&FetchModSync, std::move(promise), std::string_view(url), downloadPath);
	t.join();
	return f.get();
}

bool ModDownloader::IsModLegit(fs::path modPath, std::string_view expectedChecksum)
{
	if (strstr(GetCommandLineA(), VERIFICATION_FLAG))
	{
		DevMsg(eLog::NS, "Bypassing mod verification due to flag set up.\n");
		return true;
	}

	NTSTATUS status;
	BCRYPT_ALG_HANDLE algorithmHandle = NULL;
	BCRYPT_HASH_HANDLE hashHandle = NULL;
	std::vector<uint8_t> hash;
	DWORD hashLength = 0;
	DWORD resultLength = 0;
	std::stringstream ss;

	constexpr size_t bufferSize {1 << 12};
	std::vector<char> buffer(bufferSize, '\0');
	std::ifstream fp(modPath.generic_string(), std::ios::binary);

	// Open an algorithm handle
	// This sample passes BCRYPT_HASH_REUSABLE_FLAG with BCryptAlgorithmProvider(...) to load a provider which supports reusable hash
	status = BCryptOpenAlgorithmProvider(
		&algorithmHandle, // Alg Handle pointer
		BCRYPT_SHA256_ALGORITHM, // Cryptographic Algorithm name (null terminated unicode string)
		NULL, // Provider name; if null, the default provider is loaded
		BCRYPT_HASH_REUSABLE_FLAG); // Flags; Loads a provider which supports reusable hash
	if (!NT_SUCCESS(status))
	{
		goto cleanup;
	}

	// Obtain the length of the hash
	status = BCryptGetProperty(
		algorithmHandle, // Handle to a CNG object
		BCRYPT_HASH_LENGTH, // Property name (null terminated unicode string)
		(PBYTE)&hashLength, // Address of the output buffer which recieves the property value
		sizeof(hashLength), // Size of the buffer in bytes
		&resultLength, // Number of bytes that were copied into the buffer
		0); // Flags
	if (!NT_SUCCESS(status))
	{
		// goto cleanup;
		return false;
	}

	// Create a hash handle
	status = BCryptCreateHash(
		algorithmHandle, // Handle to an algorithm provider
		&hashHandle, // A pointer to a hash handle - can be a hash or hmac object
		NULL, // Pointer to the buffer that recieves the hash/hmac object
		0, // Size of the buffer in bytes
		NULL, // A pointer to a key to use for the hash or MAC
		0, // Size of the key in bytes
		0); // Flags
	if (!NT_SUCCESS(status))
	{
		goto cleanup;
	}

	// Hash archive content
	if (!fp.is_open())
	{
		Error(eLog::NS, NO_ERROR, "Unable to open archive.\n");
		return false;
	}
	fp.seekg(0, fp.beg);
	while (fp.good())
	{
		fp.read(buffer.data(), bufferSize);
		std::streamsize bytesRead = fp.gcount();
		if (bytesRead > 0)
		{
			status = BCryptHashData(hashHandle, (PBYTE)buffer.data(), bytesRead, 0);
			if (!NT_SUCCESS(status))
			{
				goto cleanup;
			}
		}
	}

	hash = std::vector<uint8_t>(hashLength);

	// Obtain the hash of the message(s) into the hash buffer
	status = BCryptFinishHash(
		hashHandle, // Handle to the hash or MAC object
		hash.data(), // A pointer to a buffer that receives the hash or MAC value
		hashLength, // Size of the buffer in bytes
		0); // Flags
	if (!NT_SUCCESS(status))
	{
		goto cleanup;
	}

	// Convert hash to string using bytes raw values
	ss << std::hex << std::setfill('0');
	for (int i = 0; i < hashLength; i++)
	{
		ss << std::hex << std::setw(2) << static_cast<int>(hash.data()[i]);
	}

	DevMsg(eLog::NS, "Expected checksum: %s\n", expectedChecksum.data());
	DevMsg(eLog::NS, "Computed checksum: %s\n", ss.str().c_str());
	return expectedChecksum.compare(ss.str()) == 0;

cleanup:
	if (NULL != hashHandle)
	{
		BCryptDestroyHash(hashHandle); // Handle to hash/MAC object which needs to be destroyed
	}

	if (NULL != algorithmHandle)
	{
		BCryptCloseAlgorithmProvider(
			algorithmHandle, // Handle to the algorithm provider which needs to be closed
			0); // Flags
	}

	return false;
}

bool ModDownloader::IsModAuthorized(std::string_view modName, std::string_view modVersion)
{
	if (strstr(GetCommandLineA(), VERIFICATION_FLAG))
	{
		DevMsg(eLog::NS, "Bypassing mod verification due to flag set up.\n");
		return true;
	}

	if (!verifiedMods.contains(modName.data()))
	{
		return false;
	}

	std::unordered_map<std::string, VerifiedModVersion> versions = verifiedMods[modName.data()].versions;
	return versions.count(modVersion.data()) != 0;
}

void ModDownloader::ExtractMod(fs::path modPath)
{
	unzFile file;
	std::string name;
	fs::path modDirectory;

	file = unzOpen(modPath.generic_string().c_str());
	if (file == NULL)
	{
		Error(eLog::NS, NO_ERROR, "Cannot open archive located at %s.\n", modPath.c_str());
		goto EXTRACTION_CLEANUP;
	}

	unz_global_info64 gi;
	int status;
	status = unzGetGlobalInfo64(file, &gi);
	if (status != UNZ_OK)
	{
		Error(eLog::NS, NO_ERROR, "Failed getting information from archive (error code: %i)\n", status);
		goto EXTRACTION_CLEANUP;
	}

	// Mod directory name (removing the ".zip" fom the archive name)
	name = modPath.filename().string();
	name = name.substr(0, name.length() - 4);
	modDirectory = GetRemoteModFolderPath() / name;

	for (int i = 0; i < gi.number_entry; i++)
	{
		char zipFilename[256];
		unz_file_info64 fileInfo;
		status = unzGetCurrentFileInfo64(file, &fileInfo, zipFilename, sizeof(zipFilename), NULL, 0, NULL, 0);

		// Extract file
		{
			std::error_code ec;
			fs::path fileDestination = modDirectory / zipFilename;
			DevMsg(eLog::NS, "=> %s\n", fileDestination.c_str());

			// Create parent directory if needed
			if (!std::filesystem::exists(fileDestination.parent_path()))
			{
				DevMsg(eLog::NS, "Parent directory does not exist, creating it.\n", fileDestination.generic_string());
				if (!std::filesystem::create_directories(fileDestination.parent_path(), ec) && ec.value() != 0)
				{
					Error(eLog::NS, NO_ERROR, "Parent directory (%s) creation failed.\n", fileDestination.parent_path().c_str());
					goto EXTRACTION_CLEANUP;
				}
			}

			// If current file is a directory, create directory...
			if (fileDestination.generic_string().back() == '/')
			{
				// Create directory
				if (!std::filesystem::create_directory(fileDestination, ec) && ec.value() != 0)
				{
					Error(eLog::NS, NO_ERROR, "Directory creation failed: %s\n", ec.message().c_str());
					goto EXTRACTION_CLEANUP;
				}
			}
			// ...else create file
			else
			{
				// Ensure file is in zip archive
				if (unzLocateFile(file, zipFilename, 0) != UNZ_OK)
				{
					Error(eLog::NS, NO_ERROR, "File \"%s\" was not found in archive.\n", zipFilename);
					goto EXTRACTION_CLEANUP;
				}

				// Create file
				const int bufferSize = 8192;
				void* buffer;
				int err = UNZ_OK;
				FILE* fout = NULL;

				// Open zip file to prepare its extraction
				status = unzOpenCurrentFile(file);
				if (status != UNZ_OK)
				{
					Error(eLog::NS, NO_ERROR, "Could not open file %s from archive.\n", zipFilename);
					goto EXTRACTION_CLEANUP;
				}

				// Create destination file
				fout = fopen(fileDestination.generic_string().c_str(), "wb");
				if (fout == NULL)
				{
					Error(eLog::NS, NO_ERROR, "Failed creating destination file.\n");
					goto EXTRACTION_CLEANUP;
				}

				// Allocate memory for buffer
				buffer = (void*)malloc(bufferSize);
				if (buffer == NULL)
				{
					Error(eLog::NS, NO_ERROR, "Error while allocating memory.\n");
					goto EXTRACTION_CLEANUP;
				}

				// Extract file to destination
				do
				{
					err = unzReadCurrentFile(file, buffer, bufferSize);
					if (err < 0)
					{
						Error(eLog::NS, NO_ERROR, "error %i with zipfile in unzReadCurrentFile\n", err);
						break;
					}
					if (err > 0)
					{
						if (fwrite(buffer, (unsigned)err, 1, fout) != 1)
						{
							Error(eLog::NS, NO_ERROR, "error in writing extracted file\n");
							err = UNZ_ERRNO;
							break;
						}
					}
				} while (err > 0);

				if (err != UNZ_OK)
				{
					Error(eLog::NS, NO_ERROR, "An error occurred during file extraction (code: %i)\n", err);
					goto EXTRACTION_CLEANUP;
				}
				err = unzCloseCurrentFile(file);
				if (err != UNZ_OK)
				{
					Error(eLog::NS, NO_ERROR, "error %i with zipfile in unzCloseCurrentFile\n", err);
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
				Error(eLog::NS, NO_ERROR, "Error while browsing archive files (error code: %i).\n", status);
				goto EXTRACTION_CLEANUP;
			}
		}
	}

EXTRACTION_CLEANUP:
	if (unzClose(file) != MZ_OK)
	{
		Error(eLog::NS, NO_ERROR, "Failed closing mod archive after extraction.\n");
	}
}

void ModDownloader::DownloadMod(std::string modName, std::string modVersion)
{
	// Check if mod can be auto-downloaded
	if (!IsModAuthorized(std::string_view(modName), std::string_view(modVersion)))
	{
		Warning(eLog::NS, "Tried to download a mod that is not verified, aborting.\n");
		return;
	}

	std::thread requestThread(
		[this, modName, modVersion]()
		{
			fs::path archiveLocation;

			// Download mod archive
			std::string expectedHash = verifiedMods[modName].versions[modVersion].checksum;
			std::optional<fs::path> fetchingResult = FetchModFromDistantStore(std::string_view(modName), std::string_view(modVersion));
			if (!fetchingResult.has_value())
			{
				Error(eLog::NS, NO_ERROR, "Something went wrong while fetching archive, aborting.\n");
				goto REQUEST_END_CLEANUP;
			}
			archiveLocation = fetchingResult.value();
			if (!IsModLegit(archiveLocation, std::string_view(expectedHash)))
			{
				Warning(eLog::NS, "Archive hash does not match expected checksum, aborting.\n");
				goto REQUEST_END_CLEANUP;
			}

			// Extract downloaded mod archive
			ExtractMod(archiveLocation);

		REQUEST_END_CLEANUP:
			try
			{
				remove(archiveLocation);
			}
			catch (const std::exception& a)
			{
				Error(eLog::NS, NO_ERROR, "Error while removing downloaded archive: %s\n", a.what());
			}

			DevMsg(eLog::NS, "Done downloading %s.\n", modName.c_str());
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
