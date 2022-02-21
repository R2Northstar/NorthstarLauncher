#include "pch.h"
#include <iostream>
#include "rpakfilesystem.h"
#include "hookutils.h"
#include "modmanager.h"
#include "gameutils.h"

typedef void* (*LoadCommonPaksForMapType)(char* map);
LoadCommonPaksForMapType LoadCommonPaksForMap;

typedef void* (*LoadPakSyncType)(const char* path, void* unknownSingleton, int flags);
typedef void* (*LoadPakAsyncType)(const char* path, void* unknownSingleton, int flags, void* callback0, void* callback1);

// RPAK LOAD TEST
bool RPakLoadTestEnabled() { return CommandLine() && CommandLine()->CheckParm("-rpakloadtest"); }

// there are more i'm just too lazy to add
struct PakLoadFuncs
{
	void* unknown[2];
	LoadPakSyncType func2;
	LoadPakAsyncType LoadPakAsync;
};

PakLoadFuncs* g_pakLoadApi;
std::vector<std::string> rpaksLoaded;
void** pUnknownPakLoadSingleton;

void LoadPakAsync(const char* path) { g_pakLoadApi->LoadPakAsync(path, *pUnknownPakLoadSingleton, 2, nullptr, nullptr); }

void LoadCommonPaksForMapHook(char* map)
{
	// q: do i have to unload these
	using iterator = fs::directory_iterator;


	LoadCommonPaksForMap(map);

	if (RPakLoadTestEnabled())
	{
		const iterator end;
		std::cout << "trying to load " << map << " - scanning for additional rpaks:" << std::endl;
		for (iterator iter{ "r2/paks/Win64/" }; iter != end; ++iter)
		{
			std::string mapName = iter->path().filename().string().substr(0, iter->path().filename().string().size() - 5);
			// don't load starpaks/txts/etc bruh
			if (iter->path().string().substr(iter->path().string().size() - 5, iter->path().string().size()) != ".rpak")
				continue;

			// no (XX).rpak rpaks, thanks.
			if (iter->path().string()[iter->path().string().size() - 6] == ')')
				continue;

			if (strcmp(mapName.substr(0, 2).c_str(), "sp") && strcmp(mapName.substr(0, 2).c_str(), "mp"))
				continue;

			std::string curMapName = map;

			// don't load the rpak twice cause that crashes the game
			if (mapName.find(curMapName) != std::string::npos)
			{
				//spdlog::info("skipping " + iter->path().string().substr(3, 1000) + " since it's already getting loaded.");
				continue;
			}
			// no loadscreens ;)
			if (mapName.find("loadscreen") != std::string::npos)
			{
				//spdlog::info("skipping " + iter->path().string().substr(3, 1000) + " since it's a loadscreen.");
				continue;
			}

			spdlog::info("loading " + iter->path().string().substr(3, 1000));

			rpaksLoaded.push_back(iter->path().string().substr(3, 1000));
			// begin hoping protocol
			//g_pakLoadApi->func2(const_cast<char*>(iter->path().string().substr(3, 1000).c_str()), *pUnknownPakLoadSingleton, 2);
		}
	}


	// for sp => mp conversions, load the sp rpaks
	if (!strcmp(map, "mp_skyway_v1") || !strcmp(map, "mp_crashsite") || !strcmp(map, "mp_hub_timeshift"))
		map[0] = 's';
}

void InitialiseEngineRpakFilesystem(HMODULE baseAddress)
{
	g_pakLoadApi = (PakLoadFuncs*)((char*)baseAddress + 0x5BED78);
	pUnknownPakLoadSingleton = (void**)((char*)baseAddress + 0x7C5E20);

	HookEnabler hook;
	ENABLER_CREATEHOOK(hook, (char*)baseAddress + 0x15AD20, &LoadCommonPaksForMapHook, reinterpret_cast<LPVOID*>(&LoadCommonPaksForMap));
}