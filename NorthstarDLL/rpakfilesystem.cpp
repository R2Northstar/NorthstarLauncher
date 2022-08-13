#include "pch.h"
#include "rpakfilesystem.h"
#include "modmanager.h"
#include "dedicated.h"

AUTOHOOK_INIT()

// there are more i'm just too lazy to add
struct PakLoadFuncs
{
	void* unk0[2];
	void* (*LoadPakSync)(const char* pPath, void* unknownSingleton, int flags);
	int (*LoadPakAsync)(const char* pPath, void* unknownSingleton, int flags, void* callback0, void* callback1);
	void* unk1[2];
	void* (*UnloadPak)(int iPakHandle, void* callback);
	void* unk2[6];
	long long (*FileExsits)(const char* path);//unsure
	void* unk3[10];
	void* (*ReadFullFileFromDisk)(const char* pPath, void* a2);
};

PakLoadFuncs* g_pakLoadApi;

PakLoadManager* g_pPakLoadManager;
void** pUnknownPakLoadSingleton;
void PakLoadManager::LoadPakSync(const char* path)
{
	g_pakLoadApi->LoadPakSync(path, *pUnknownPakLoadSingleton, 0);
}

void PakLoadManager::LoadPakAsync(const char* path, bool bMarkForUnload)
{
	int handle = g_pakLoadApi->LoadPakAsync(path, *pUnknownPakLoadSingleton, 2, nullptr, nullptr);

	if (bMarkForUnload)
		m_pakHandlesToUnload.push_back(handle);
}

void PakLoadManager::UnloadPaks()
{
	for (int pakHandle : m_pakHandlesToUnload)
		g_pakLoadApi->UnloadPak(pakHandle, nullptr);

	m_pakHandlesToUnload.clear();
}

long long PakLoadManager::FileExists(const char* path)
{
	return g_pakLoadApi->FileExsits(path);
}

void HandlePakAliases(char** map)
{
	// convert the pak being loaded to it's aliased one, e.g. aliasing mp_hub_timeshift => sp_hub_timeshift
	for (int64_t i = g_pModManager->m_LoadedMods.size() - 1; i > -1; i--)
	{
		Mod* mod = &g_pModManager->m_LoadedMods[i];
		if (!mod->m_bEnabled)
			continue;

		if (mod->RpakAliases.find(*map) != mod->RpakAliases.end())
		{
			*map = &mod->RpakAliases[*map][0];
			return;
		}
	}
}

void LoadPreloadPaks()
{
	// note, loading from ./ is necessary otherwise paks will load from gamedir/r2/paks
	for (Mod& mod : g_pModManager->m_LoadedMods)
	{
		if (!mod.m_bEnabled)
			continue;

		// need to get a relative path of mod to mod folder
		fs::path modPakPath("./" / mod.m_ModDirectory / "paks");

		for (ModRpakEntry& pak : mod.Rpaks)
			if (pak.m_bAutoLoad)
				g_pPakLoadManager->LoadPakAsync((modPakPath / pak.m_sPakName).string().c_str(), false);
	}
}

void LoadCustomMapPaks(char** pakName, bool* bNeedToFreePakName)
{
	// whether the vanilla game has this rpak
	bool bHasOriginalPak = fs::exists(fs::path("r2/paks/Win64/") / *pakName);

	// note, loading from ./ is necessary otherwise paks will load from gamedir/r2/paks
	for (Mod& mod : g_pModManager->m_LoadedMods)
	{
		if (!mod.m_bEnabled)
			continue;

		// need to get a relative path of mod to mod folder
		fs::path modPakPath("./" / mod.m_ModDirectory / "paks");

		for (ModRpakEntry& pak : mod.Rpaks)
		{
			if (!pak.m_bAutoLoad && !pak.m_sPakName.compare(*pakName))
			{
				// if the game doesn't have the original pak, let it handle loading this one as if it was the one it was loading originally
				if (!bHasOriginalPak)
				{
					std::string path = (modPakPath / pak.m_sPakName).string();
					*pakName = new char[path.size() + 1];
					strcpy(*pakName, &path[0]);
					(*pakName)[path.size()] = '\0';

					bHasOriginalPak = true;
					*bNeedToFreePakName =
						true; // we can't free this memory until we're done with the pak, so let whatever's calling this deal with it
				}
				else
					g_pPakLoadManager->LoadPakAsync((modPakPath / pak.m_sPakName).string().c_str(), true);
			}
		}
	}
}

HOOK(LoadPakAsyncHook, LoadPakAsync,
int,, (char* pPath, void* unknownSingleton, int flags, void* callback0, void* callback1))
{
	HandlePakAliases(&pPath);

	bool bNeedToFreePakName = false;

	static bool bShouldLoadPaks = true;
	if (bShouldLoadPaks)
	{
		// make a copy of the path for comparing to determine whether we should load this pak on dedi, before it could get overwritten by LoadCustomMapPaks
		std::string originalPath(pPath);

		// disable preloading while we're doing this
		bShouldLoadPaks = false;

		LoadPreloadPaks();
		LoadCustomMapPaks(&pPath, &bNeedToFreePakName);

		bShouldLoadPaks = true;

		// do this after custom paks load and in bShouldLoadPaks so we only ever call this on the root pakload call
		// todo: could probably add some way to flag custom paks to not be loaded on dedicated servers in rpak.json
		if (IsDedicatedServer() && strncmp(&originalPath[0], "common", 6)) // dedicated only needs common and common_mp
		{
			spdlog::info("Not loading pak {} for dedicated server", originalPath);
			return -1;	
		}
	}

	int ret = LoadPakAsync(pPath, unknownSingleton, flags, callback0, callback1);
	spdlog::info("LoadPakAsync {} {}", pPath, ret);

	if (bNeedToFreePakName)
		delete[] pPath;

	return ret;
}

HOOK(UnloadPakHook, UnloadPak,
void*,, (int iPakHandle, void* callback))
{
	static bool bShouldUnloadPaks = true;
	if (bShouldUnloadPaks)
	{
		bShouldUnloadPaks = false;
		g_pPakLoadManager->UnloadPaks();
		bShouldUnloadPaks = true;
	}

	spdlog::info("UnloadPak {}", iPakHandle);
	return UnloadPak(iPakHandle, callback);
}

// we hook this exclusively for resolving stbsp paths, but seemingly it's also used for other stuff like vpk and rpak loads
// possibly just async loading altogether?

HOOK(ReadFullFileFromDiskHook, ReadFullFileFromDisk, 
void*, , (const char* pPath, void* a2))
{
	fs::path path(pPath);
	char* allocatedNewPath = nullptr;

	if (path.extension() == ".stbsp")
	{
		fs::path filename = path.filename();
		spdlog::info("LoadStreamBsp: {}", filename.string());

		// resolve modded stbsp path so we can load mod stbsps

		auto modFile = g_pModManager->m_ModFiles.find(g_pModManager->NormaliseModFilePath(fs::path("maps" / filename)));
		if (modFile != g_pModManager->m_ModFiles.end())
		{
			// need to allocate a new string for this
			std::string newPath = (modFile->second.m_pOwningMod->m_ModDirectory / "mod" / modFile->second.m_Path).string();
			allocatedNewPath = new char[newPath.size() + 1];
			strncpy_s(allocatedNewPath, newPath.size() + 1, newPath.c_str(), newPath.size());
			pPath = allocatedNewPath;
		}
	}

	void* ret = ReadFullFileFromDisk(pPath, a2);
	if (allocatedNewPath)
		delete[] allocatedNewPath;

	return ret;
}


ON_DLL_LOAD("engine.dll", RpakFilesystem, (CModule module))
{
	AUTOHOOK_DISPATCH();

	g_pPakLoadManager = new PakLoadManager;

	g_pakLoadApi = module.Offset(0x5BED78).Deref().As<PakLoadFuncs*>();
	pUnknownPakLoadSingleton = module.Offset(0x7C5E20).As<void**>();

	LoadPakAsyncHook.Dispatch(g_pakLoadApi->LoadPakAsync);
	UnloadPakHook.Dispatch(g_pakLoadApi->UnloadPak);
	ReadFullFileFromDiskHook.Dispatch(g_pakLoadApi->ReadFullFileFromDisk);
}