#include "rpakfilesystem.h"
#include "mods/modmanager.h"
#include "dedicated/dedicated.h"
#include "core/tier0.h"
#include "util/utils.h"

AUTOHOOK_INIT()

struct PakLoadFuncs
{
	void (*InitRpakSystem)();
	void (*AddAssetLoaderWithJobDetails)(/*assetTypeHeader*/ void*, uint32_t, int);
	void (*AddAssetLoader)(/*assetTypeHeader*/ void*);
	__int64 (*LoadRpakFileAsync)(const char* pPath, void* allocator, int flags);
	void (*LoadRpakFile)(const char*, __int64(__fastcall*)(), __int64, void(__cdecl*)());
	__int64 qword28;
	void (*UnloadPak)(int iPakHandle, void* callback);
	__int64 qword38;
	__int64 qword40;
	__int64 qword48;
	__int64 qword50;
	FARPROC (*GetDllCallback)(__int16 a1, const CHAR* a2);
	__int64 (*GetAssetByHash)(__int64 hash);
	__int64 (*GetAssetByName)(const char* a1);
	__int64 qword70;
	__int64 qword78;
	__int64 qword80;
	__int64 qword88;
	__int64 qword90;
	char gap98[0x28];
	void* (*OpenFile)(const char* pPath);
	__int64 CloseFile;
	__int64 qwordD0;
	__int64 FileReadAsync;
	__int64 ComplexFileReadAsync;
	__int64 GetReadJobState;
	__int64 WaitForFileReadJobComplete;
	__int64 CancelFileReadJob;
	__int64 CancelFileReadJobAsync;
	__int64 qword108;
};
static_assert(sizeof(PakLoadFuncs) == 0x110);
static_assert(offsetof(PakLoadFuncs, InitRpakSystem) == 0x0);
static_assert(offsetof(PakLoadFuncs, AddAssetLoaderWithJobDetails) == 0x8);
static_assert(offsetof(PakLoadFuncs, AddAssetLoader) == 0x10);
static_assert(offsetof(PakLoadFuncs, LoadRpakFileAsync) == 0x18);
static_assert(offsetof(PakLoadFuncs, LoadRpakFile) == 0x20);
static_assert(offsetof(PakLoadFuncs, UnloadPak) == 0x30);
static_assert(offsetof(PakLoadFuncs, GetDllCallback) == 0x58);
static_assert(offsetof(PakLoadFuncs, GetAssetByHash) == 0x60);
static_assert(offsetof(PakLoadFuncs, GetAssetByName) == 0x68);
static_assert(offsetof(PakLoadFuncs, OpenFile) == 0xC0);
static_assert(offsetof(PakLoadFuncs, CloseFile) == 0xC8);
static_assert(offsetof(PakLoadFuncs, FileReadAsync) == 0xD8);
static_assert(offsetof(PakLoadFuncs, ComplexFileReadAsync) == 0xE0);
static_assert(offsetof(PakLoadFuncs, GetReadJobState) == 0xE8);
static_assert(offsetof(PakLoadFuncs, WaitForFileReadJobComplete) == 0xF0);
static_assert(offsetof(PakLoadFuncs, CancelFileReadJob) == 0xF8);
static_assert(offsetof(PakLoadFuncs, CancelFileReadJobAsync) == 0x100);

PakLoadFuncs* g_pakLoadApi;

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
unsigned int (*o_pGetPakPatchNumber)(const char* pPakPath) = nullptr;

void NewPakLoadManager::UnloadAllModPaks()
{
	NS::log::rpak->info("Reloading RPaks on next map load...");
	for (auto& modPak : m_modPaks)
	{
		modPak.m_markedForDelete = true;
	}
	// clean up any paks that are both marked for unload and already unloaded
	CleanUpUnloadedPaks();
	m_forceReloadOnMapLoad = true;
}

void NewPakLoadManager::TrackModPaks(Mod& mod)
{
	fs::path modPakPath("./" / mod.m_ModDirectory / "paks");

	for (auto& modRpakEntry : mod.Rpaks)
	{
		ModPak pak;
		pak.m_modName = mod.Name;
		pak.m_path = (modPakPath / modRpakEntry.m_pakName).string();
		pak.m_pathHash = STR_HASH(pak.m_path);

		pak.m_mapRegex = modRpakEntry.m_loadRegex;
		pak.m_dependentPakHash = modRpakEntry.m_dependentPakHash;

		// todo: prevent duplicates?

		m_modPaks.push_back(pak);
	}
}

void NewPakLoadManager::CleanUpUnloadedPaks()
{
	auto predicate = [](ModPak& pak) -> bool
	{
		return pak.m_markedForDelete && pak.m_handle == -1;
	};

	m_modPaks.erase(std::remove_if(m_modPaks.begin(), m_modPaks.end(), predicate), m_modPaks.end());
}

void NewPakLoadManager::UnloadMarkedPaks()
{
	++m_reentranceCounter;
	ScopeGuard guard([&](){ --m_reentranceCounter; });

	(*o_CModelLoader_UnreferenceAllModels)(*modelLoader);
	(*o_cleanMaterialSystemStuff)();

	for (auto& modPak : m_modPaks)
	{
		if (modPak.m_handle == -1 || !modPak.m_markedForDelete)
			continue;

		g_pakLoadApi->UnloadPak(modPak.m_handle, *o_cleanMaterialSystemStuff);
		modPak.m_handle = -1;
	}
}

void NewPakLoadManager::LoadModPaksForMap(const char* mapName)
{
	++m_reentranceCounter;
	ScopeGuard guard([&]() { --m_reentranceCounter; });

	for (auto& modPak : m_modPaks)
	{
		// don't load paks that are already loaded
		if (modPak.m_handle != -1)
			continue;
		std::cmatch matches;
		if (!std::regex_match(mapName, matches, modPak.m_mapRegex))
			continue;

		modPak.m_handle = g_pakLoadApi->LoadRpakFileAsync(modPak.m_path.c_str(), *rpakMemoryAllocator, 7);
		m_mapPaks.push_back(modPak.m_pathHash);
	}
}

void NewPakLoadManager::UnloadModPaks()
{
	++m_reentranceCounter;
	ScopeGuard guard([&]() { --m_reentranceCounter; });

	(*o_CModelLoader_UnreferenceAllModels)(*modelLoader);
	(*o_cleanMaterialSystemStuff)();

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

	// If this has happened, we may have leaked a pak?
	// It basically means that none of the entries in m_modPaks matched the hash in m_mapPaks so we didn't end up unloading it
	assert_msg(m_mapPaks.size() == 0, "Not all map paks were unloaded?");
}

void NewPakLoadManager::OnPakLoaded(std::string& originalPath, std::string& resultingPath, int resultingHandle)
{
	if (IsVanillaCall())
	{
		// add entry to loaded vanilla rpaks
		m_vanillaPaks.emplace_back(originalPath, resultingHandle);
	}

	LoadDependentPaks(resultingPath, resultingHandle);
}

void NewPakLoadManager::OnPakUnloading(int handle)
{
	UnloadDependentPaks(handle);

	if (IsVanillaCall())
	{
		// remove entry from loaded vanilla rpaks
		auto predicate = [handle](std::pair<std::string, int>& pair) -> bool { return pair.second == handle; };

		m_vanillaPaks.erase(std::remove_if(m_vanillaPaks.begin(), m_vanillaPaks.end(), predicate), m_vanillaPaks.end());

		// no need to handle aliasing here, if vanilla wants it gone, it's gone
	}
	else
	{
		// handle the potential unloading of an aliased vanilla rpak (we aliased it, and we are now unloading the alias, so we should load the vanilla one again)
		for (auto& [path, resultingHandle] : m_vanillaPaks)
		{
			if (resultingHandle != handle)
				continue;

			// load vanilla rpak
		}
	}

	// set handle of the mod pak (if any) that has this handle for proper tracking
	for (auto& modPak : m_modPaks)
	{
		if (modPak.m_handle == handle)
			modPak.m_handle = -1;
	}
}

void NewPakLoadManager::LoadDependentPaks(std::string& path, int handle)
{
	++m_reentranceCounter;
	ScopeGuard guard([&]() { --m_reentranceCounter; });

	const size_t hash = STR_HASH(path);
	for (auto& modPak : m_modPaks)
	{
		if (modPak.m_handle != -1)
			continue;
		if (modPak.m_dependentPakHash != hash)
			continue;

		// load pak
		modPak.m_handle = g_pakLoadApi->LoadRpakFileAsync(modPak.m_path.c_str(), *rpakMemoryAllocator, 7);
		m_dependentPaks.emplace_back(handle, hash);
	}
}

void NewPakLoadManager::UnloadDependentPaks(int handle)
{
	++m_reentranceCounter;
	ScopeGuard guard([&]() { --m_reentranceCounter; });

	auto predicate = [&](std::pair<int, size_t>& pair) -> bool
	{
		if (pair.first != handle)
			return false;

		for (auto& modPak : m_modPaks)
		{
			if (modPak.m_pathHash != pair.second || modPak.m_handle == -1)
				continue;

			// unload pak
			g_pakLoadApi->UnloadPak(modPak.m_handle, *o_cleanMaterialSystemStuff);
			modPak.m_handle = -1;
		}

		return true;
	};
	m_dependentPaks.erase(std::remove_if(m_dependentPaks.begin(), m_dependentPaks.end(), predicate), m_dependentPaks.end());
}

void NewPakLoadManager::LoadPreloadPaks()
{
	++m_reentranceCounter;
	ScopeGuard guard([&]() { --m_reentranceCounter; });

	for (auto& modPak : m_modPaks)
	{
		if (modPak.m_markedForDelete || !modPak.m_preload)
			continue;

		modPak.m_handle = g_pakLoadApi->LoadRpakFileAsync(modPak.m_path.c_str(), *rpakMemoryAllocator, 7);
	}
}

void* NewPakLoadManager::OpenFile(const char* path)
{
	return g_pakLoadApi->OpenFile(path);
}

// whether the vanilla game has this rpak
static bool VanillaHasPak(const char* pakName)
{
	fs::path originalPath = fs::path("./r2/paks/Win64") / pakName;
	unsigned int highestPatch = o_pGetPakPatchNumber(pakName);
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

void NewPakLoadManager::FixupPakPath(std::string& pakPath)
{
	if (VanillaHasPak(pakPath.c_str()))
		return;

	// todo: this doesn't adhere to load priority. Does it need to?
	for (ModPak& modPak : m_modPaks)
	{
		if (modPak.m_markedForDelete)
			continue;

		fs::path modPakFilename = fs::path(modPak.m_path).filename();
		if (pakPath == modPakFilename.string())
		{
			pakPath = modPak.m_path;
			return;
		}
	}
}

bool (*o_LoadMapRpaks)(char* mapPath) = nullptr;
bool h_LoadMapRpaks(char* mapPath)
{
	// unload all mod rpaks that are marked for unload
	g_pNewPakLoadManager->UnloadMarkedPaks();
	g_pNewPakLoadManager->CleanUpUnloadedPaks();

	// strip file extension
	const std::string mapName = fs::path(mapPath).replace_extension().string();

	// load mp_common, sp_common etc.
	o_LoadGametypeSpecificRpaks(mapName.c_str());

	// unload old modded map paks
	g_pNewPakLoadManager->UnloadModPaks();
	// load modded map paks
	g_pNewPakLoadManager->LoadModPaksForMap(mapName.c_str());

	// don't load/unload anything when going to the lobby, presumably to save load times when going back to the same map
	if (!g_pNewPakLoadManager->GetForceReloadOnMapLoad() && !strcmp("mp_lobby", mapName.c_str()))
		return false;

	char mapRpakStr[272];
	snprintf(mapRpakStr, 272, "%s.rpak", mapName.c_str());

	// if level being loaded is the same as current level, do nothing
	if (!g_pNewPakLoadManager->GetForceReloadOnMapLoad() && !strcmp(mapRpakStr, currentMapRpakPath))
		return true;

	strcpy(currentMapRpakPath, mapRpakStr);

	(*o_cleanMaterialSystemStuff)();
	o_loadlevelLoadscreen(mapName.c_str());

	// unload old map rpaks
	int curHandle = *currentMapRpakHandle;
	int curPatchHandle = *currentMapPatchRpakHandle;
	if (curHandle != -1)
	{
		(*o_CModelLoader_UnreferenceAllModels)(*modelLoader);
		(*o_cleanMaterialSystemStuff)();
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

	*currentMapRpakHandle = g_pakLoadApi->LoadRpakFileAsync(mapRpakStr, *rpakMemoryAllocator, 7);

	// load special _patch rpak (seemingly used for dev things?)
	char levelPatchRpakStr[272];
	snprintf(levelPatchRpakStr, 272, "%s_patch.rpak", mapName.c_str());
	*currentMapPatchRpakHandle = g_pakLoadApi->LoadRpakFileAsync(levelPatchRpakStr, *rpakMemoryAllocator, 7);

	// we just reloaded the paks, so we don't need to force it again
	g_pNewPakLoadManager->SetForceReloadOnMapLoad(false);
	return true;
}

// todo: deprecate?
static void HandlePakAliases(std::string& originalPath)
{
	// convert the pak being loaded to it's aliased one, e.g. aliasing mp_hub_timeshift => sp_hub_timeshift
	for (int64_t i = g_pModManager->m_LoadedMods.size() - 1; i > -1; i--)
	{
		Mod* mod = &g_pModManager->m_LoadedMods[i];
		if (!mod->m_bEnabled)
			continue;

		if (mod->RpakAliases.find(originalPath) != mod->RpakAliases.end())
		{
			originalPath = &mod->RpakAliases[originalPath][0];
			return;
		}
	}
}

// clang-format off
HOOK(LoadPakAsyncHook, LoadPakAsync,
int, __fastcall, (const char* pPath, void* memoryAllocator, int flags))
// clang-format on
{
	// make a copy of the path for comparing to determine whether we should load this pak on dedi, before it could get overwritten
	std::string originalPath(pPath);

	std::string resultingPath(pPath);
	HandlePakAliases(resultingPath);

	if (g_pNewPakLoadManager->IsVanillaCall())
	{
		g_pNewPakLoadManager->LoadPreloadPaks();
		g_pNewPakLoadManager->FixupPakPath(resultingPath);

		// do this after custom paks load and in bShouldLoadPaks so we only ever call this on the root pakload call
		// todo: could probably add some way to flag custom paks to not be loaded on dedicated servers in rpak.json

		// dedicated only needs common, common_mp, common_sp, and sp_<map> rpaks
		// sp_<map> rpaks contain tutorial ghost data
		// sucks to have to load the entire rpak for that but sp was never meant to be done on dedi
		if (IsDedicatedServer() &&
			(CommandLine()->CheckParm("-nopakdedi") || strncmp(&originalPath[0], "common", 6) && strncmp(&originalPath[0], "sp_", 3)))
		{
			NS::log::rpak->info("Not loading pak {} for dedicated server", originalPath);
			return -1;
		}
	}

	int iPakHandle = LoadPakAsync(resultingPath.c_str(), memoryAllocator, flags);
	NS::log::rpak->info("LoadPakAsync {} {}", resultingPath, iPakHandle);

	g_pNewPakLoadManager->OnPakLoaded(originalPath, resultingPath, iPakHandle);

	return iPakHandle;
}

// clang-format off
HOOK(UnloadPakHook, UnloadPak,
void*, __fastcall, (int nPakHandle, void* pCallback))
// clang-format on
{
	g_pNewPakLoadManager->OnPakUnloading(nPakHandle);

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

ON_DLL_LOAD("engine.dll", RpakFilesystem, (CModule module))
{
	AUTOHOOK_DISPATCH();

	g_pNewPakLoadManager = new NewPakLoadManager;

	g_pakLoadApi = module.Offset(0x5BED78).Deref().RCast<PakLoadFuncs*>();
	pUnknownPakLoadSingleton = module.Offset(0x7C5E20).RCast<void**>();

	LoadPakAsyncHook.Dispatch((LPVOID*)g_pakLoadApi->LoadRpakFileAsync);
	UnloadPakHook.Dispatch((LPVOID*)g_pakLoadApi->UnloadPak);
	OpenFileHook.Dispatch((LPVOID*)g_pakLoadApi->OpenFile);

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

	// kinda bad, hooking things in rtech in an engine callback but it seems fine for now
	CModule rtechModule(GetModuleHandleA("rtech_game.dll"));
	o_pGetPakPatchNumber = rtechModule.Offset(0x9A00).RCast<decltype(o_pGetPakPatchNumber)>();
}
