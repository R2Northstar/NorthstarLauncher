#include "pch.h"
#include "masterserver.cpp"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;

Document verifiedModsJson;

void _FetchVerifiedModsList() {
	CURL* curl = curl_easy_init();

	std::string readBuffer;
	curl_easy_setopt(curl, CURLOPT_URL, fmt::format("{}/client/verifiedmods", Cvar_ns_masterserver_hostname->GetString()).c_str());
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteToStringBufferCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

	CURLcode result = curl_easy_perform(curl);

	if (result == CURLcode::CURLE_OK)
	{
		verifiedModsJson.Parse(readBuffer.c_str());

		if (verifiedModsJson.HasParseError())
		{
			spdlog::error(
				"Failed reading masterserver verified mods response: encountered parse error \"{}\"",
				rapidjson::GetParseError_En(verifiedModsJson.GetParseError()));
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
				spdlog::error( std::string(verifiedModsJson["error"]["enum"].GetString()) );
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
}

std::string GetVerifiedModsList() {
	if (verifiedModsJson.IsNull())
	{
		_FetchVerifiedModsList();
	}
	StringBuffer buffer;
	Writer<StringBuffer> writer(buffer);
	verifiedModsJson.Accept(writer);
	return buffer.GetString();
}

void DownloadMod(char* dependencyString) {
	// TODO check if mod is already present
	// TODO check if mod is verified (throw if not)
	// TODO download zip in temporary folder
	// TODO move mod to mods/ folder
}
