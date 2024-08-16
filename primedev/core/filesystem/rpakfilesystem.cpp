#include "rpakfilesystem.h"
#include "mods/modmanager.h"
#include "dedicated/dedicated.h"
#include "core/tier0.h"

AUTOHOOK_INIT()

struct PakLoadFuncs
{
	void(__fastcall* initRpakSystem)();
	void(__fastcall* addAssetLoaderWithJobDetails)(/*assetTypeHeader*/ void*, uint32_t, int);
	void(__fastcall* addAssetLoader)(/*assetTypeHeader*/ void*);
	__int64(__fastcall* loadRpakFileAsync)(const char* pPath, void* allocator, int flags);
	void(__fastcall* loadRpakFile)(const char*, __int64(__fastcall*)(), __int64, void(__cdecl*)());
	__int64 qword28;
	void (*UnloadPak)(int iPakHandle, void* callback);
	__int64 qword38;
	__int64 qword40;
	__int64 qword48;
	__int64 qword50;
	FARPROC(__fastcall* getDllCallback)(__int16 a1, const CHAR* a2);
	__int64(__fastcall* getAssetByHash)(__int64 hash);
	__int64(__fastcall* getAssetByName)(const char* a1);
	__int64 qword70;
	__int64 qword78;
	__int64 qword80;
	__int64 qword88;
	__int64 qword90;
	char gap98[40];
	void* (*openFile)(const char* pPath);
	__int64 closeFile;
	__int64 qwordD0;
	__int64 fileReadAsync;
	__int64 complexFileReadAsync;
	__int64 getReadJobState;
	__int64 waitForFileReadJobComplete;
	__int64 cancelFileReadJob;
	__int64 cancelFileReadJobAsync;
	__int64 qword108;
};

PakLoadFuncs* g_pakLoadApi;

PakLoadManager* g_pPakLoadManager;
NewPakLoadManager* g_pNewPakLoadManager;
void** pUnknownPakLoadSingleton;

static char* currentMapRpakPath = nullptr;
static int* currentMapRpakHandle = nullptr;
static int* currentMapPatchRpakHandle = nullptr;
static /*CModelLoader*/ void** modelLoader = nullptr;
static void** rpakMemoryAllocator = nullptr;

static __int64 (*o_LoadGametypeSpecificRpaks)(const char* levelName) = nullptr;
static __int64 (**o_cleanMaterialSystemStuff)() = nullptr;
static __int64 (**o_CModelLoader_UnreferenceAllModels)(/*CModelLoader*/ void* a1) = nullptr;
static char* (*o_loadlevelLoadscreen)(const char* levelName) = nullptr;

int PakLoadManager::LoadPakAsync(const char* pPath, const ePakLoadSource nLoadSource)
{
	int nHandle = g_pakLoadApi->loadRpakFileAsync(pPath, *pUnknownPakLoadSingleton, 2);

	// set the load source of the pak we just loaded
	if (nHandle != -1)
		GetPakInfo(nHandle)->m_nLoadSource = nLoadSource;

	return nHandle;
}

void PakLoadManager::UnloadPak(const int nPakHandle, void* pCallback)
{
	g_pakLoadApi->UnloadPak(nPakHandle, pCallback);
}

void PakLoadManager::UnloadMapPaks(void* pCallback)
{
	// copy since UnloadPak removes from the map
	std::map<int, LoadedPak> loadedPaksCopy = m_vLoadedPaks;
	for (auto& pair : loadedPaksCopy)
		if (pair.second.m_nLoadSource == ePakLoadSource::MAP)
			UnloadPak(pair.first, pCallback);
}

LoadedPak* PakLoadManager::TrackLoadedPak(ePakLoadSource nLoadSource, int nPakHandle, size_t nPakNameHash)
{
	LoadedPak pak;
	pak.m_nLoadSource = nLoadSource;
	pak.m_nPakHandle = nPakHandle;
	pak.m_nPakNameHash = nPakNameHash;

	m_vLoadedPaks.insert(std::make_pair(nPakHandle, pak));
	return &m_vLoadedPaks.at(nPakHandle);
}

void PakLoadManager::RemoveLoadedPak(int nPakHandle)
{
	m_vLoadedPaks.erase(nPakHandle);
}

LoadedPak* PakLoadManager::GetPakInfo(const int nPakHandle)
{
	return &m_vLoadedPaks.at(nPakHandle);
}

int PakLoadManager::GetPakHandle(const size_t nPakNameHash)
{
	for (auto& pair : m_vLoadedPaks)
		if (pair.second.m_nPakNameHash == nPakNameHash)
			return pair.first;

	return -1;
}

int PakLoadManager::GetPakHandle(const char* pPath)
{
	return GetPakHandle(STR_HASH(pPath));
}

void* PakLoadManager::LoadFile(const char* path)
{
	return g_pakLoadApi->openFile(path);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////

void NewPakLoadManager::UnloadAllModdedPaks()
{
	NS::log::rpak->info("Reloading RPaks on next map load...");
	for (auto& modPak : m_modPaks)
	{
		modPak.m_markedForUnload = true;
	}
	// clean up any paks that are both marked for unload and already unloaded
	CleanUpUnloadedPaks();
}

void NewPakLoadManager::TrackModPaks(Mod& mod)
{
	fs::path modPakPath("./" / mod.m_ModDirectory / "paks");

	for (auto& modRpakEntry : mod.Rpaks)
	{
		ModPak pak;
		pak.m_modName = mod.Name;
		pak.m_path = (modPakPath / modRpakEntry.m_sPakName).string();
		pak.m_pathHash = STR_HASH(pak.m_path);

		pak.m_loadOnMultiplayerMaps = modRpakEntry.m_loadOnMP;
		pak.m_loadOnSingleplayerMaps = modRpakEntry.m_loadOnSP;
		pak.m_targetMap = modRpakEntry.m_targetMap;

		// todo: prevent duplicates?

		m_modPaks.push_back(pak);
	}
}

void NewPakLoadManager::CleanUpUnloadedPaks()
{
	auto predicate = [](ModPak& pak) -> bool
	{
		return pak.m_markedForUnload && pak.m_handle == -1;
	};

	m_modPaks.erase(std::remove_if(m_modPaks.begin(), m_modPaks.end(), predicate), m_modPaks.end());
}

void NewPakLoadManager::UnloadMarkedPaks()
{
	m_vanillaCall = false;

	(*o_CModelLoader_UnreferenceAllModels)(*modelLoader);
	(*o_cleanMaterialSystemStuff)();

	for (auto& modPak : m_modPaks)
	{
		if (modPak.m_handle == -1 || !modPak.m_markedForUnload)
			continue;

		g_pakLoadApi->UnloadPak(modPak.m_handle, *o_cleanMaterialSystemStuff);
		modPak.m_handle = -1;
	}

	m_vanillaCall = true;
}

void NewPakLoadManager::LoadMapPaks(const char* mapName)
{
	m_vanillaCall = false;
	for (auto& modPak : m_modPaks)
	{
		// don't load paks that are already loaded
		if (modPak.m_handle != -1)
			continue;
		if (modPak.m_targetMap != mapName)
			continue;

		modPak.m_handle = g_pakLoadApi->loadRpakFileAsync(modPak.m_path.c_str(), *rpakMemoryAllocator, 7);
		m_mapPaks.push_back(modPak.m_pathHash);
	}
	m_vanillaCall = true;
}

void NewPakLoadManager::UnloadMapPaks()
{
	m_vanillaCall = false;
	for (auto& modPak : m_modPaks)
	{
		for (auto it = m_mapPaks.begin(); it != m_mapPaks.end(); ++it)
		{
			if (*it != modPak.m_pathHash)
				continue;

			m_mapPaks.erase(it, it + 1);
			g_pakLoadApi->UnloadPak(modPak.m_handle, *o_cleanMaterialSystemStuff);
			modPak.m_handle = -1;
			break;
		}
	}
	m_vanillaCall = true;
}

bool (*o_LoadMapRpaks)(char* mapPath) = nullptr;
bool h_LoadMapRpaks(char* mapPath)
{
	// unload all mod rpaks that are marked for unload
	g_pNewPakLoadManager->UnloadMarkedPaks();
	g_pNewPakLoadManager->CleanUpUnloadedPaks();

	// strip file extension
	const std::string mapName = fs::path(mapPath).replace_extension().string();

	// todo: load modded gametype specific rpaks here as well
	o_LoadGametypeSpecificRpaks(mapName.c_str());

	if (!strcmp("mp_lobby", mapName.c_str())) // nothing to load for mp_lobby
		return false;

	char mapRpakStr[272];
	snprintf(mapRpakStr, 272, "%s.rpak", mapName.c_str());
	// if level being loaded is the same as current level, do nothing
	// todo: do stuff if we have reloaded mods
	if (!strcmp(mapRpakStr, currentMapRpakPath))
		return true;

	strcpy(currentMapRpakPath, mapRpakStr);

	(*o_cleanMaterialSystemStuff)();
	o_loadlevelLoadscreen(mapName.c_str());
	int curHandle = *currentMapRpakHandle;
	int curPatchHandle = *currentMapPatchRpakHandle;
	if (curHandle != -1)
	{
		(*o_CModelLoader_UnreferenceAllModels)(*modelLoader);
		(*o_cleanMaterialSystemStuff)();

		g_pNewPakLoadManager->UnloadMapPaks();
		g_pakLoadApi->UnloadPak(curHandle, *o_cleanMaterialSystemStuff);
		*currentMapRpakHandle = -1;
	}
	if (curPatchHandle != -1)
	{
		(*o_CModelLoader_UnreferenceAllModels)(*modelLoader);
		(*o_cleanMaterialSystemStuff)();
		g_pakLoadApi->UnloadPak(curPatchHandle, *o_cleanMaterialSystemStuff);
		*currentMapPatchRpakHandle = -1;
	}

	*currentMapRpakHandle = g_pakLoadApi->loadRpakFileAsync(mapRpakStr, *rpakMemoryAllocator, 7);
	g_pNewPakLoadManager->LoadMapPaks(mapName.c_str());

	// load special _patch rpak (seemingly used for dev things?)
	char levelPatchRpakStr[272];
	snprintf(levelPatchRpakStr, 272, "%s_patch.rpak", mapName.c_str());
	// todo: do we need to do modded map patch rpaks? they can just be modded map rpaks
	*currentMapPatchRpakHandle = g_pakLoadApi->loadRpakFileAsync(levelPatchRpakStr, *rpakMemoryAllocator, 7);

	return true;
}

unsigned int (*o_pGetPakPatchNumber)(char* pPakPath) = nullptr;
unsigned int h_GetPakPatchNumber(char* pPakPath)
{
	__int64 ret = o_pGetPakPatchNumber(pPakPath);
	NS::log::rpak->info("Highest patch number for {} is {}", pPakPath, ret);
	return ret;
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
				g_pPakLoadManager->LoadPakAsync((modPakPath / pak.m_sPakName).string().c_str(), ePakLoadSource::CONSTANT);
	}
}

void LoadPostloadPaks(const char* pPath)
{
	// note, loading from ./ is necessary otherwise paks will load from gamedir/r2/paks
	for (Mod& mod : g_pModManager->m_LoadedMods)
	{
		if (!mod.m_bEnabled)
			continue;

		// need to get a relative path of mod to mod folder
		fs::path modPakPath("./" / mod.m_ModDirectory / "paks");

		for (ModRpakEntry& pak : mod.Rpaks)
			if (pak.m_sLoadAfterPak == pPath)
				g_pPakLoadManager->LoadPakAsync((modPakPath / pak.m_sPakName).string().c_str(), ePakLoadSource::CONSTANT);
	}
}

// whether the vanilla game has this rpak
bool VanillaHasPak(char* pakName)
{
	fs::path originalPath = fs::path("./r2/paks/Win64") / pakName;
	unsigned int highestPatch = h_GetPakPatchNumber(pakName);
	if (highestPatch)
	{
		// add the patch path to the extension
		char buf[16];
		snprintf(buf, sizeof(buf), "(%02u).rpak", highestPatch);
		// remove the .rpak and add the new suffix
		originalPath = originalPath.replace_extension().string() + buf;
	}
	else
	{
		originalPath /= pakName;
	}

	return fs::exists(originalPath);
}

void LoadCustomMapPaks(char** pakName, bool* bNeedToFreePakName)
{
	bool bHasOriginalPak = VanillaHasPak(*pakName);

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
					g_pPakLoadManager->LoadPakAsync((modPakPath / pak.m_sPakName).string().c_str(), ePakLoadSource::MAP);
			}
		}
	}
}

// clang-format off
HOOK(LoadPakAsyncHook, LoadPakAsync,
int, __fastcall, (char* pPath, void* memoryAllocator, int flags))
// clang-format on
{
	HandlePakAliases(&pPath);

	// dont load the pak if it's currently loaded already
	size_t nPathHash = STR_HASH(pPath);
	if (g_pPakLoadManager->GetPakHandle(nPathHash) != -1)
		return -1;

	bool bNeedToFreePakName = false;

	static bool bShouldLoadPaks = true;
	if (bShouldLoadPaks)
	{
		// make a copy of the path for comparing to determine whether we should load this pak on dedi, before it could get overwritten by
		// LoadCustomMapPaks
		std::string originalPath(pPath);

		// disable preloading while we're doing this
		bShouldLoadPaks = false;

		LoadPreloadPaks();
		LoadCustomMapPaks(&pPath, &bNeedToFreePakName);

		bShouldLoadPaks = true;

		// do this after custom paks load and in bShouldLoadPaks so we only ever call this on the root pakload call
		// todo: could probably add some way to flag custom paks to not be loaded on dedicated servers in rpak.json

		// dedicated only needs common, common_mp, common_sp, and sp_<map> rpaks
		// sp_<map> rpaks contain tutorial ghost data
		// sucks to have to load the entire rpak for that but sp was never meant to be done on dedi
		if (IsDedicatedServer() &&
			(CommandLine()->CheckParm("-nopakdedi") || strncmp(&originalPath[0], "common", 6) && strncmp(&originalPath[0], "sp_", 3)))
		{
			if (bNeedToFreePakName)
				delete[] pPath;

			NS::log::rpak->info("Not loading pak {} for dedicated server", originalPath);
			return -1;
		}
	}

	int iPakHandle = LoadPakAsync(pPath, memoryAllocator, flags);
	NS::log::rpak->info("LoadPakAsync {} {}", pPath, iPakHandle);

	// trak the pak
	g_pPakLoadManager->TrackLoadedPak(ePakLoadSource::UNTRACKED, iPakHandle, nPathHash);
	LoadPostloadPaks(pPath);

	if (bNeedToFreePakName)
		delete[] pPath;

	return iPakHandle;
}

// clang-format off
HOOK(UnloadPakHook, UnloadPak,
void*, __fastcall, (int nPakHandle, void* pCallback))
// clang-format on
{
	ePakLoadSource loadSource = g_pPakLoadManager->GetPakInfo(nPakHandle)->m_nLoadSource;
	// stop tracking the pak
	g_pPakLoadManager->RemoveLoadedPak(nPakHandle);

	static bool bShouldUnloadPaks = true;
	if (bShouldUnloadPaks)
	{
		bShouldUnloadPaks = false;
		if (loadSource == ePakLoadSource::MAP)
			g_pPakLoadManager->UnloadMapPaks(pCallback);
		bShouldUnloadPaks = true;
	}

	NS::log::rpak->info("UnloadPak {}", nPakHandle);
	return UnloadPak(nPakHandle, pCallback);
}

// we hook this exclusively for resolving stbsp paths, but seemingly it's also used for other stuff like vpk, rpak, mprj and starpak loads
// tbh this actually might be for memory mapped files or something, would make sense i think
// clang-format off
HOOK(OpenFileHook, o_OpenFile, 
void*, __fastcall, (const char* pPath, void* pCallback))
// clang-format on
{
	fs::path path(pPath);
	std::string newPath = "";
	fs::path filename = path.filename();

	if (path.extension() == ".stbsp")
	{
		if (IsDedicatedServer())
			return nullptr;

		NS::log::rpak->info("LoadStreamBsp: {}", filename.string());

		// resolve modded stbsp path so we can load mod stbsps
		auto modFile = g_pModManager->m_ModFiles.find(g_pModManager->NormaliseModFilePath(fs::path("maps" / filename)));
		if (modFile != g_pModManager->m_ModFiles.end())
		{
			newPath = (modFile->second.m_pOwningMod->m_ModDirectory / "mod" / modFile->second.m_Path).string();
			pPath = newPath.c_str();
		}
	}
	else if (path.extension() == ".starpak")
	{
		if (IsDedicatedServer())
			return nullptr;

		// code for this is mostly stolen from above

		// unfortunately I can't find a way to get the rpak that is causing this function call, so I have to
		// store them on mod init and then compare the current path with the stored paths

		// game adds r2\ to every path, so assume that a starpak path that begins with r2\paks\ is a vanilla one
		// modded starpaks will be in the mod's paks folder (but can be in folders within the paks folder)

		// this might look a bit confusing, but its just an iterator over the various directories in a path.
		// path.begin() being the first directory, r2 in this case, which is guaranteed anyway,
		// so increment the iterator with ++ to get the first actual directory, * just gets the actual value
		// then we compare to "paks" to determine if it's a vanilla rpak or not
		if (*++path.begin() != "paks")
		{
			// remove the r2\ from the start used for path lookups
			std::string starpakPath = path.string().substr(3);
			// hash the starpakPath to compare with stored entries
			size_t hashed = STR_HASH(starpakPath);

			// loop through all loaded mods
			for (Mod& mod : g_pModManager->m_LoadedMods)
			{
				// ignore non-loaded mods
				if (!mod.m_bEnabled)
					continue;

				// loop through the stored starpak paths
				for (size_t hash : mod.StarpakPaths)
				{
					if (hash == hashed)
					{
						// construct new path
						newPath = (mod.m_ModDirectory / "paks" / starpakPath).string();
						// set path to the new path
						pPath = newPath.c_str();
						goto LOG_STARPAK;
					}
				}
			}
		}

	LOG_STARPAK:
		NS::log::rpak->info("LoadStreamPak: {}", filename.string());
	}

	return o_OpenFile(pPath, pCallback);
}

//void ConCommand_reload_models(const CCommand& args)
//{
//	//(*o_CModelLoader_UnreferenceAllModels)(*modelLoader);
//	(*o_cleanMaterialSystemStuff)();
//}

ON_DLL_LOAD("engine.dll", RpakFilesystem, (CModule module))
{
	AUTOHOOK_DISPATCH();

	g_pPakLoadManager = new PakLoadManager;
	g_pNewPakLoadManager = new NewPakLoadManager;

	//RegisterConCommand("reload_models", ConCommand_reload_models, "test, absolutely crashes the game", FCVAR_CLIENTDLL);

	g_pakLoadApi = module.Offset(0x5BED78).Deref().RCast<PakLoadFuncs*>();
	pUnknownPakLoadSingleton = module.Offset(0x7C5E20).RCast<void**>();

	LoadPakAsyncHook.Dispatch((LPVOID*)g_pakLoadApi->loadRpakFileAsync);
	UnloadPakHook.Dispatch((LPVOID*)g_pakLoadApi->UnloadPak);
	OpenFileHook.Dispatch((LPVOID*)g_pakLoadApi->openFile);

	currentMapRpakPath = module.Offset(0x1315C3E0).RCast<decltype(currentMapRpakPath)>();
	currentMapRpakHandle = module.Offset(0x7CB5A0).RCast<decltype(currentMapRpakHandle)>();
	currentMapPatchRpakHandle = module.Offset(0x7CB5A4).RCast<decltype(currentMapPatchRpakHandle)>();
	modelLoader = module.Offset(0x7C4AC0).RCast<decltype(modelLoader)>();
	rpakMemoryAllocator = module.Offset(0x7C5E20).RCast<decltype(rpakMemoryAllocator)>();

	o_LoadGametypeSpecificRpaks = module.Offset(0x15AD20).RCast<decltype(o_LoadGametypeSpecificRpaks)>();
	o_cleanMaterialSystemStuff = module.Offset(0x12A11F00).RCast<decltype(o_cleanMaterialSystemStuff)>();
	o_CModelLoader_UnreferenceAllModels = module.Offset(0x5ED580).RCast<decltype(o_CModelLoader_UnreferenceAllModels)>(); // this is part of a vftable, todo: map it in a different file
	o_loadlevelLoadscreen = module.Offset(0x15A810).RCast<decltype(o_loadlevelLoadscreen)>();

	o_LoadMapRpaks = module.Offset(0x15A8C0).RCast<decltype(o_LoadMapRpaks)>();
	HookAttach(&(PVOID&)o_LoadMapRpaks, (PVOID)h_LoadMapRpaks);

	CModule rtechModule(GetModuleHandleA("rtech_game.dll"));
	o_pGetPakPatchNumber = rtechModule.Offset(0x9A00).RCast<decltype(o_pGetPakPatchNumber)>();
	HookAttach(&(PVOID&)o_pGetPakPatchNumber, (PVOID)h_GetPakPatchNumber);
}
