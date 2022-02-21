// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "dllmain.h"
#include <array>
#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>
#include <windows.h>
#include <chrono>

#include "library/discord.h"

#if defined(_WIN32)
#pragma pack(push, 1)



struct BitmapImageHeader
{
	uint32_t const structSize{sizeof(BitmapImageHeader)};
	int32_t width{0};
	int32_t height{0};
	uint16_t const planes{1};
	uint16_t const bpp{32};
	uint32_t const pad0{0};
	uint32_t const pad1{0};
	uint32_t const hres{2835};
	uint32_t const vres{2835};
	uint32_t const pad4{0};
	uint32_t const pad5{0};

	BitmapImageHeader& operator=(BitmapImageHeader const&) = delete;
};

struct BitmapFileHeader
{
	uint8_t const magic0{'B'};
	uint8_t const magic1{'M'};
	uint32_t size{0};
	uint32_t const pad{0};
	uint32_t const offset{sizeof(BitmapFileHeader) + sizeof(BitmapImageHeader)};

	BitmapFileHeader& operator=(BitmapFileHeader const&) = delete;
};
#pragma pack(pop)
#endif

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

struct DiscordState
{
	discord::User currentUser;

	std::unique_ptr<discord::Core> core;
};

namespace
{
volatile bool interrupted{false};
}

#define EXPORT __declspec(dllexport)

#include "../NorthstarDedicatedTest/state.h"

GameState* gameStatePTR = 0;

extern "C" EXPORT void initializePlugin(GameState* gameStatePTR_external)
{
	gameStatePTR = gameStatePTR_external;
	std::thread discord(main, 0, (char**)0);
	discord.detach();
}

DiscordState state{};
bool wasInGame;

int main(int, char**)
{

	SetEnvironmentVariable(L"DISCORD_INSTANCE_ID", L"0");
	SetConsoleTitle(L"Northstar");

	discord::Core* core{};
	auto result = discord::Core::Create(941428101429231617, DiscordCreateFlags_NoRequireDiscord, &core);
	state.core.reset(core);
	if (!state.core)
	{
		std::cout << "Failed to instantiate discord core! (err " << static_cast<int>(result) << ")\n";
		std::exit(-1);
	}
	std::cout << "Instantiated discord core!\n";

	state.core->SetLogHook(
		discord::LogLevel::Debug, [](discord::LogLevel level, const char* message)
		{ std::cerr << "Log(" << static_cast<uint32_t>(level) << "): " << message << "\n"; });

	

	discord::Activity activity{};

	const auto p1 = std::chrono::system_clock::now().time_since_epoch();
	activity.GetTimestamps().SetStart(std::chrono::duration_cast<std::chrono::seconds>(p1).count());

	std::string gameState = (*gameStatePTR).playlistDisplayName;

	activity.SetDetails("");
	activity.SetState("Loading...");
	activity.GetAssets().SetLargeImage("northstar");
	activity.GetAssets().SetLargeText("");
	activity.SetType(discord::ActivityType::Playing);

	activity.GetParty().GetSize().SetCurrentSize(4);
	activity.GetParty().GetSize().SetMaxSize(12);


	state.core->ActivityManager().UpdateActivity(
		activity,
		[](discord::Result result)
		{
			printf(((result == discord::Result::Ok) ? "Succeeded" : "Failed"));
			printf("\n");
		});

	std::signal(SIGINT, [](int) { interrupted = true; });

	do
	{
		state.core->RunCallbacks();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		std::string details = "Score: ";

		
		activity.SetState((*gameStatePTR).playlistDisplayName.c_str());
		activity.GetAssets().SetLargeImage((*gameStatePTR).map.c_str());
		activity.GetAssets().SetLargeText((*gameStatePTR).mapDisplayName.c_str());

		if ((*gameStatePTR).map == "")
		{
			activity.GetParty().GetSize().SetCurrentSize(0);
			activity.GetParty().GetSize().SetMaxSize(0);
			activity.SetDetails("Main Menu");
			activity.SetState("On Main Menu");
			activity.GetAssets().SetLargeImage("northstar");
			activity.GetAssets().SetLargeText("Titanfall 2 + Northstar");
			activity.GetAssets().SetSmallImage("");
			activity.GetAssets().SetSmallText("");
			activity.GetTimestamps().SetEnd(0);
			if (wasInGame)
			{
				const auto p1 = std::chrono::system_clock::now().time_since_epoch();
				activity.GetTimestamps().SetStart(std::chrono::duration_cast<std::chrono::seconds>(p1).count());
				wasInGame = false;
			}
		}
		else if ((*gameStatePTR).map == "mp_lobby")
		{
			activity.SetState("In the Lobby");
			activity.GetParty().GetSize().SetCurrentSize(0);
			activity.GetParty().GetSize().SetMaxSize(0);
			activity.SetDetails("Lobby");
			activity.GetAssets().SetLargeImage("northstar");
			activity.GetAssets().SetLargeText("Titanfall 2 + Northstar");
			activity.GetAssets().SetSmallImage("");
			activity.GetAssets().SetSmallText("");
			activity.GetTimestamps().SetEnd(0);
			if (wasInGame)
			{
				const auto p1 = std::chrono::system_clock::now().time_since_epoch();
				activity.GetTimestamps().SetStart(std::chrono::duration_cast<std::chrono::seconds>(p1).count());
				wasInGame = false;
			}
		}
		else
		{
			if (!(*gameStatePTR).loading)
			{
				activity.GetParty().GetSize().SetCurrentSize((*gameStatePTR).players);
				activity.GetParty().GetSize().SetMaxSize((*gameStatePTR).serverInfo.maxPlayers);

				if ((*gameStatePTR).ourScore == (*gameStatePTR).highestScore)
				{
					details += std::to_string((*gameStatePTR).ourScore) + " - " + std::to_string((*gameStatePTR).secondHighestScore);
				}
				else
				{
					details += std::to_string((*gameStatePTR).ourScore) + " - " + std::to_string((*gameStatePTR).highestScore);
				}

				details += " (First to " + std::to_string((*gameStatePTR).serverInfo.scoreLimit) + ")";
				activity.SetDetails(details.c_str());
				activity.GetTimestamps().SetEnd((*gameStatePTR).serverInfo.endTime);
				activity.GetTimestamps().SetStart(0);
				wasInGame = true;
			}
			else
			{
				activity.GetParty().GetSize().SetCurrentSize(0);
				activity.GetParty().GetSize().SetMaxSize(0);
				activity.SetDetails("Loading...");
			}
		}


		state.core->ActivityManager().UpdateActivity(
			activity, [](discord::Result result){});

	} while (!interrupted);

	return 0;
}