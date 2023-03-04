#include "audio.h"
#include "dedicated/dedicated.h"
#include "core/convar/convar.h"

#include "rapidjson/error/en.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <random>

AUTOHOOK_INIT()

extern "C"
{
	// should be called only in LoadSampleMetadata_Hook
	extern void* __fastcall Audio_GetParentEvent();
}

ConVar* Cvar_ns_print_played_sounds;

CustomAudioManager g_CustomAudioManager;

EventOverrideData::EventOverrideData()
{
	spdlog::warn("Initialised struct EventOverrideData without any data!");
	LoadedSuccessfully = false;
}

// Empty stereo 48000 WAVE file
unsigned char EMPTY_WAVE[45] = {0x52, 0x49, 0x46, 0x46, 0x25, 0x00, 0x00, 0x00, 0x57, 0x41, 0x56, 0x45, 0x66, 0x6D, 0x74,
								0x20, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x44, 0xAC, 0x00, 0x00, 0x88, 0x58,
								0x01, 0x00, 0x02, 0x00, 0x10, 0x00, 0x64, 0x61, 0x74, 0x61, 0x74, 0x00, 0x00, 0x00, 0x00};

EventOverrideData::EventOverrideData(const std::string& data, const fs::path& path)
{
	if (data.length() <= 0)
	{
		spdlog::error("Failed reading audio override file {}: file is empty", path.string());
		return;
	}

	fs::path samplesFolder = path;
	samplesFolder = samplesFolder.replace_extension();

	if (!fs::exists(samplesFolder))
	{
		spdlog::error(
			"Failed reading audio override file {}: samples folder doesn't exist; should be named the same as the definition file without "
			"JSON extension.",
			path.string());
		return;
	}

	rapidjson_document dataJson;
	dataJson.Parse<rapidjson::ParseFlag::kParseCommentsFlag | rapidjson::ParseFlag::kParseTrailingCommasFlag>(data);

	// fail if parse error
	if (dataJson.HasParseError())
	{
		spdlog::error(
			"Failed reading audio override file {}: encountered parse error \"{}\" at offset {}",
			path.string(),
			GetParseError_En(dataJson.GetParseError()),
			dataJson.GetErrorOffset());
		return;
	}

	// fail if it's not a json obj (could be an array, string, etc)
	if (!dataJson.IsObject())
	{
		spdlog::error("Failed reading audio override file {}: file is not a JSON object", path.string());
		return;
	}

	// fail if no event ids given
	if (!dataJson.HasMember("EventId"))
	{
		spdlog::error("Failed reading audio override file {}: JSON object does not have the EventId property", path.string());
		return;
	}

	// array of event ids
	if (dataJson["EventId"].IsArray())
	{
		for (auto& eventId : dataJson["EventId"].GetArray())
		{
			if (!eventId.IsString())
			{
				spdlog::error(
					"Failed reading audio override file {}: EventId array has a value of invalid type, all must be strings", path.string());
				return;
			}

			EventIds.push_back(eventId.GetString());
		}
	}
	// singular event id
	else if (dataJson["EventId"].IsString())
	{
		EventIds.push_back(dataJson["EventId"].GetString());
	}
	// incorrect type
	else
	{
		spdlog::error(
			"Failed reading audio override file {}: EventId property is of invalid type (must be a string or an array of strings)",
			path.string());
		return;
	}

	if (dataJson.HasMember("EventIdRegex"))
	{
		// array of event id regex
		if (dataJson["EventIdRegex"].IsArray())
		{
			for (auto& eventId : dataJson["EventIdRegex"].GetArray())
			{
				if (!eventId.IsString())
				{
					spdlog::error(
						"Failed reading audio override file {}: EventIdRegex array has a value of invalid type, all must be strings",
						path.string());
					return;
				}

				const std::string& regex = eventId.GetString();

				try
				{
					EventIdsRegex.push_back({regex, std::regex(regex)});
				}
				catch (...)
				{
					spdlog::error("Malformed regex \"{}\" in audio override file {}", regex, path.string());
					return;
				}
			}
		}
		// singular event id regex
		else if (dataJson["EventIdRegex"].IsString())
		{
			const std::string& regex = dataJson["EventIdRegex"].GetString();
			try
			{
				EventIdsRegex.push_back({regex, std::regex(regex)});
			}
			catch (...)
			{
				spdlog::error("Malformed regex \"{}\" in audio override file {}", regex, path.string());
				return;
			}
		}
		// incorrect type
		else
		{
			spdlog::error(
				"Failed reading audio override file {}: EventIdRegex property is of invalid type (must be a string or an array of strings)",
				path.string());
			return;
		}
	}

	if (dataJson.HasMember("AudioSelectionStrategy"))
	{
		if (!dataJson["AudioSelectionStrategy"].IsString())
		{
			spdlog::error("Failed reading audio override file {}: AudioSelectionStrategy property must be a string", path.string());
			return;
		}

		std::string strategy = dataJson["AudioSelectionStrategy"].GetString();

		if (strategy == "sequential")
		{
			Strategy = AudioSelectionStrategy::SEQUENTIAL;
		}
		else if (strategy == "random")
		{
			Strategy = AudioSelectionStrategy::RANDOM;
		}
		else
		{
			spdlog::error(
				"Failed reading audio override file {}: AudioSelectionStrategy string must be either \"sequential\" or \"random\"",
				path.string());
			return;
		}
	}

	// load samples
	for (fs::directory_entry file : fs::recursive_directory_iterator(samplesFolder))
	{
		if (file.is_regular_file() && file.path().extension().string() == ".wav")
		{
			std::string pathString = file.path().string();

			// Open the file.
			std::ifstream wavStream(pathString, std::ios::binary);

			if (wavStream.fail())
			{
				spdlog::error("Failed reading audio sample {}", file.path().string());
				continue;
			}

			// Get file size.
			wavStream.seekg(0, std::ios::end);
			size_t fileSize = wavStream.tellg();
			wavStream.close();

			// Allocate enough memory for the file.
			// blank out the memory for now, then read it later
			uint8_t* data = new uint8_t[fileSize];
			memcpy(data, EMPTY_WAVE, sizeof(EMPTY_WAVE));
			Samples.push_back({fileSize, std::unique_ptr<uint8_t[]>(data)});

			// thread off the file read
			// should we spawn one thread per read? or should there be a cap to the number of reads at once?
			std::thread readThread(
				[pathString, fileSize, data]
				{
					std::shared_lock lock(g_CustomAudioManager.m_loadingMutex);
					std::ifstream wavStream(pathString, std::ios::binary);

					// would be weird if this got hit, since it would've worked previously
					if (wavStream.fail())
					{
						spdlog::error("Failed async read of audio sample {}", pathString);
						return;
					}

					// read from after the header first to preserve the empty header, then read the header last
					wavStream.seekg(0, std::ios::beg);
					wavStream.read(reinterpret_cast<char*>(data), fileSize);
					wavStream.close();

					spdlog::info("Finished async read of audio sample {}", pathString);
				});

			readThread.detach();
		}
	}

	/*
	if (dataJson.HasMember("EnableOnLoopedSounds"))
	{
		if (!dataJson["EnableOnLoopedSounds"].IsBool())
		{
			spdlog::error("Failed reading audio override file {}: EnableOnLoopedSounds property is of invalid type (must be a bool)",
	path.string()); return;
		}

		EnableOnLoopedSounds = dataJson["EnableOnLoopedSounds"].GetBool();
	}
	*/

	if (Samples.size() == 0)
		spdlog::warn("Audio override {} has no valid samples! Sounds will not play for this event.", path.string());

	spdlog::info("Loaded audio override file {}", path.string());

	LoadedSuccessfully = true;
}

bool CustomAudioManager::TryLoadAudioOverride(const fs::path& defPath)
{
	if (IsDedicatedServer())
		return true; // silently fail

	std::ifstream jsonStream(defPath);
	std::stringstream jsonStringStream;

	// fail if no audio json
	if (jsonStream.fail())
	{
		spdlog::warn("Unable to read audio override from file {}", defPath.string());
		return false;
	}

	while (jsonStream.peek() != EOF)
		jsonStringStream << (char)jsonStream.get();

	jsonStream.close();

	std::shared_ptr<EventOverrideData> data = std::make_shared<EventOverrideData>(jsonStringStream.str(), defPath);

	if (!data->LoadedSuccessfully)
		return false; // no logging, the constructor has probably already logged

	for (const std::string& eventId : data->EventIds)
	{
		spdlog::info("Registering sound event {}", eventId);
		m_loadedAudioOverrides.insert({eventId, data});
	}

	for (const auto& eventIdRegexData : data->EventIdsRegex)
	{
		spdlog::info("Registering sound event regex {}", eventIdRegexData.first);
		m_loadedAudioOverridesRegex.insert({eventIdRegexData.first, data});
	}

	return true;
}

typedef void (*MilesStopAll_Type)();
MilesStopAll_Type MilesStopAll;

void CustomAudioManager::ClearAudioOverrides()
{
	if (IsDedicatedServer())
		return;

	if (m_loadedAudioOverrides.size() > 0 || m_loadedAudioOverridesRegex.size() > 0)
	{
		// stop all miles sounds beforehand
		// miles_stop_all

		MilesStopAll();

		// this is cancer but it works
		Sleep(50);
	}

	// slightly (very) bad
	// wait for all audio reads to complete so we don't kill preexisting audio buffers as we're writing to them
	std::unique_lock lock(m_loadingMutex);

	m_loadedAudioOverrides.clear();
	m_loadedAudioOverridesRegex.clear();
}

template <typename Iter, typename RandomGenerator> Iter select_randomly(Iter start, Iter end, RandomGenerator& g)
{
	std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
	std::advance(start, dis(g));
	return start;
}

template <typename Iter> Iter select_randomly(Iter start, Iter end)
{
	static std::random_device rd;
	static std::mt19937 gen(rd());
	return select_randomly(start, end, gen);
}

bool ShouldPlayAudioEvent(const char* eventName, const std::shared_ptr<EventOverrideData>& data)
{
	std::string eventNameString = eventName;
	std::string eventNameStringBlacklistEntry = ("!" + eventNameString);

	for (const std::string& name : data->EventIds)
	{
		if (name == eventNameStringBlacklistEntry)
			return false; // event blacklisted

		if (name == "*")
		{
			// check for bad sounds I guess?
			// really feel like this should be an option but whatever
			if (!!strstr(eventName, "_amb_") || !!strstr(eventName, "_emit_") || !!strstr(eventName, "amb_"))
				return false; // would play static noise, I hate this
		}
	}

	return true; // good to go
}

// forward declare
bool __declspec(noinline) __fastcall LoadSampleMetadata_Internal(
	uintptr_t parentEvent, void* sample, void* audioBuffer, unsigned int audioBufferLength, int audioType);

// DO NOT TOUCH THIS FUNCTION
// The actual logic of it in a separate function (forcefully not inlined) to preserve the r12 register, which holds the event pointer.
// clang-format off
AUTOHOOK(LoadSampleMetadata, mileswin64.dll + 0xF110, 
bool, __fastcall, (void* sample, void* audioBuffer, unsigned int audioBufferLength, int audioType))
// clang-format on
{
	uintptr_t parentEvent = (uintptr_t)Audio_GetParentEvent();

	// Raw source, used for voice data only
	if (audioType == 0)
		return LoadSampleMetadata(sample, audioBuffer, audioBufferLength, audioType);

	return LoadSampleMetadata_Internal(parentEvent, sample, audioBuffer, audioBufferLength, audioType);
}

// DO NOT INLINE THIS FUNCTION
// See comment below.
bool __declspec(noinline) __fastcall LoadSampleMetadata_Internal(
	uintptr_t parentEvent, void* sample, void* audioBuffer, unsigned int audioBufferLength, int audioType)
{
	char* eventName = (char*)parentEvent + 0x110;

	if (Cvar_ns_print_played_sounds->GetInt() > 0)
		spdlog::info("[AUDIO] Playing event {}", eventName);

	auto iter = g_CustomAudioManager.m_loadedAudioOverrides.find(eventName);
	std::shared_ptr<EventOverrideData> overrideData;

	if (iter == g_CustomAudioManager.m_loadedAudioOverrides.end())
	{
		// override for that specific event not found, try wildcard
		iter = g_CustomAudioManager.m_loadedAudioOverrides.find("*");

		if (iter == g_CustomAudioManager.m_loadedAudioOverrides.end())
		{
			// not found

			// try regex
			for (const auto& item : g_CustomAudioManager.m_loadedAudioOverridesRegex)
				for (const auto& regexData : item.second->EventIdsRegex)
					if (std::regex_search(eventName, regexData.second))
						overrideData = item.second;

			if (!overrideData)
				// not found either
				return LoadSampleMetadata(sample, audioBuffer, audioBufferLength, audioType);
			else
			{
				// cache found pattern to improve performance
				g_CustomAudioManager.m_loadedAudioOverrides[eventName] = overrideData;
			}
		}
		else
			overrideData = iter->second;
	}
	else
		overrideData = iter->second;

	if (!ShouldPlayAudioEvent(eventName, overrideData))
		return LoadSampleMetadata(sample, audioBuffer, audioBufferLength, audioType);

	void* data = 0;
	unsigned int dataLength = 0;

	if (overrideData->Samples.size() == 0)
	{
		// 0 samples, turn off this particular event.

		// using a dummy empty wave file
		data = EMPTY_WAVE;
		dataLength = sizeof(EMPTY_WAVE);
	}
	else
	{
		std::pair<size_t, std::unique_ptr<uint8_t[]>>* dat = NULL;

		switch (overrideData->Strategy)
		{
		case AudioSelectionStrategy::RANDOM:
			dat = &*select_randomly(overrideData->Samples.begin(), overrideData->Samples.end());
			break;
		case AudioSelectionStrategy::SEQUENTIAL:
		default:
			dat = &overrideData->Samples[overrideData->CurrentIndex++];
			if (overrideData->CurrentIndex >= overrideData->Samples.size())
				overrideData->CurrentIndex = 0; // reset back to the first sample entry
			break;
		}

		if (!dat)
			spdlog::warn("Could not get sample data from override struct for event {}! Shouldn't happen", eventName);
		else
		{
			data = dat->second.get();
			dataLength = dat->first;
		}
	}

	if (!data)
	{
		spdlog::warn("Could not fetch override sample data for event {}! Using original data instead.", eventName);
		return LoadSampleMetadata(sample, audioBuffer, audioBufferLength, audioType);
	}

	audioBuffer = data;
	audioBufferLength = dataLength;

	// most important change: set the sample class buffer so that the correct audio plays
	*(void**)((uintptr_t)sample + 0xE8) = audioBuffer;
	*(unsigned int*)((uintptr_t)sample + 0xF0) = audioBufferLength;

	// 64 - Auto-detect sample type
	bool res = LoadSampleMetadata(sample, audioBuffer, audioBufferLength, 64);
	if (!res)
		spdlog::error("LoadSampleMetadata failed! The game will crash :(");

	return res;
}

// clang-format off
AUTOHOOK(MilesLog, client.dll + 0x57DAD0, 
void, __fastcall, (int level, const char* string))
// clang-format on
{
	spdlog::info("[MSS] {} - {}", level, string);
}

ON_DLL_LOAD_CLIENT_RELIESON("client.dll", AudioHooks, ConVar, (CModule module))
{
	AUTOHOOK_DISPATCH()

	Cvar_ns_print_played_sounds = new ConVar("ns_print_played_sounds", "0", FCVAR_NONE, "");
	MilesStopAll = module.Offset(0x580850).As<MilesStopAll_Type>();
}
