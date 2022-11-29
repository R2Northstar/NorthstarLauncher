#include "pch.h"
#include "masterserver.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "squirrel.h"
#include "verifiedmods.h"

using namespace rapidjson;

Document verifiedModsJson;

// Test string used to test branch without masterserver
const char* modsTestString =
	"{"
	"\"Dinorush's LTS Rebalance\" : {\"DependencyPrefix\" : \"Dinorush-LTSRebalance\", \"Versions\" : []}, "
	"\"Dinorush.Brute4\" : {\"DependencyPrefix\" : \"Dinorush-DinorushBrute4\", \"Versions\" : [  \"1.5\", \"1.6.0\" ]}, "
	"\"Mod Settings\" : {\"DependencyPrefix\" : \"EladNLG-ModSettings\", \"Versions\" : [ \"1.0.0\", \"1.1.0\" ]}, "
	"\"Moblin.Archon\" : {\"DependencyPrefix\" : \"GalacticMoblin-MoblinArchon\", \"Versions\" : [ \"1.3.0\", \"1.3.1\" ]},"
	"\"Fifty.mp_frostbite\" : {\"DependencyPrefix\" : \"Fifty-Frostbite\", \"Versions\" : [ \"0.0.1\" ]}"
	"}";

void _FetchVerifiedModsList()
{
	spdlog::info("Requesting verified mods list from {}", Cvar_ns_masterserver_hostname->GetString());

	std::thread requestThread(
		[]()
		{
			CURL* curl = curl_easy_init();

			std::string readBuffer;
			curl_easy_setopt(curl, CURLOPT_URL, fmt::format("{}/client/verifiedmods", Cvar_ns_masterserver_hostname->GetString()).c_str());
			curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

			// TODO fetch list from masterserver
			verifiedModsJson.Parse(modsTestString);
			return;

			CURLcode result = curl_easy_perform(curl);
			spdlog::info(result);

			if (result == CURLcode::CURLE_OK)
			{
				spdlog::info("curl succeeded");

				verifiedModsJson.Parse(readBuffer.c_str());

				if (verifiedModsJson.HasParseError())
				{
					spdlog::error("Failed reading masterserver verified mods response: encountered parse error.");
					goto REQUEST_END_CLEANUP;
				}
				if (!verifiedModsJson.IsObject())
				{
					spdlog::error("Failed reading masterserver verified mods response: root object is not an object");
					goto REQUEST_END_CLEANUP;
				}
				if (verifiedModsJson.HasMember("error"))
				{
					spdlog::error("Failed reading masterserver response: got fastify error response");
					spdlog::error(readBuffer);
					if (verifiedModsJson["error"].HasMember("enum"))
						spdlog::error(std::string(verifiedModsJson["error"]["enum"].GetString()));
					else
						spdlog::error(std::string("No error message provided"));
					goto REQUEST_END_CLEANUP;
				}
			}
			else
			{
				spdlog::error("Failed requesting verified mods list: error {}", curl_easy_strerror(result));
			}

		REQUEST_END_CLEANUP:
			curl_easy_cleanup(curl);
		});
	requestThread.detach();
}

std::string GetVerifiedModsList()
{
	if (verifiedModsJson.IsNull())
	{
		_FetchVerifiedModsList();

		while (verifiedModsJson.IsNull())
		{
			spdlog::info("Wait for verified mods list to arrive...");
			Sleep(2000); // TODO do this asynchronously to avoid blocking the thread
		}

		spdlog::info("Verified mods list arrived.");
	}

	StringBuffer buffer;
	Writer<StringBuffer> writer(buffer);
	verifiedModsJson.Accept(writer);
	return buffer.GetString();
}

/**
 * Checks if a mod is verified by controlling if its name matches a key in the verified mods JSON
 * document, and if its version is included in the JSON versions list.
 */
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

size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream)
{
	size_t written;
	written = fwrite(ptr, size, nmemb, stream);
	return written;
}

void DownloadMod(char* modName, char* modVersion)
{
	if (!IsModVerified(modName, modVersion))
		return;

	// Rebuild mod dependency string.
	const Value& entry = verifiedModsJson[modName];
	std::string dependencyString = entry["DependencyPrefix"].GetString();
	GenericArray versions = entry["Versions"].GetArray();
	dependencyString.append("-");
	dependencyString.append(modVersion);

	std::thread requestThread(
		[dependencyString]()
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
		});
	requestThread.detach();
}

/**
 * Squirrel-exposed wrapper methods
 **/

ADD_SQFUNC("string", GetVerifiedModsList, "", "", ScriptContext::UI)
{
	std::string mods = GetVerifiedModsList();
	const SQChar* buffer = mods.c_str();
	g_pSquirrel<context>->pushstring(sqvm, buffer, -1);
	return SQRESULT_NOTNULL;
}

ADD_SQFUNC("bool", IsModVerified, "string modName, string modVersion", "", ScriptContext::UI)
{
	const SQChar* modName = g_pSquirrel<context>->getstring(sqvm, 1);
	const SQChar* modVersion = g_pSquirrel<context>->getstring(sqvm, 2);

	bool result = IsModVerified((char*)modName, (char*)modVersion);
	g_pSquirrel<context>->pushbool(sqvm, result);

	return SQRESULT_NOTNULL;
}
