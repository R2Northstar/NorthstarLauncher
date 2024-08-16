#pragma once

enum class ePakLoadSource
{
	UNTRACKED = -1, // not a pak we loaded, we shouldn't touch this one

	CONSTANT, // should be loaded at all times
	MAP // loaded from a map, should be unloaded when the map is unloaded
};

struct LoadedPak
{
	ePakLoadSource m_nLoadSource;
	int m_nPakHandle;
	size_t m_nPakNameHash;
};

class PakLoadManager
{
private:
	std::map<int, LoadedPak> m_vLoadedPaks {};
	std::unordered_map<size_t, int> m_HashToPakHandle {};

public:
	int LoadPakAsync(const char* pPath, const ePakLoadSource nLoadSource);
	void UnloadPak(const int nPakHandle, void* pCallback);
	void UnloadMapPaks(void* pCallback);
	void* LoadFile(const char* path); // this is a guess

	LoadedPak* TrackLoadedPak(ePakLoadSource nLoadSource, int nPakHandle, size_t nPakNameHash);
	void RemoveLoadedPak(int nPakHandle);

	LoadedPak* GetPakInfo(const int nPakHandle);

	int GetPakHandle(const size_t nPakNameHash);
	int GetPakHandle(const char* pPath);
};

struct ModPak
{
	std::string m_modName;
	std::string m_path;
	size_t m_pathHash;

	bool m_loadOnMultiplayerMaps;
	bool m_loadOnSingleplayerMaps;
	std::string m_targetMap;

	// if this is set, the Pak will be unloaded on next map load
	bool m_markedForUnload = false;
	int m_handle = -1;
};

/*
* [X] on mod reload, mark all paks for unloading
* [ ] on mod load, read rpak.json and track rpaks
* [ ] on map change, load the correct mod rpaks
* [ ] on map change, unload paks that are marked for unloading
* [ ] on pak load, add to vanilla tracked paks (if static bool is true)
* [ ] on pak unload, unload dependent rpaks first
* [ ] on pak unload, if pak was aliasing a vanilla pak away, load the vanilla pak (if static bool is false)
*/

class NewPakLoadManager
{
public:
	// Marks all mod Paks to be unloaded on next map load.
	// Also cleans up any mod Paks that are already unloaded.
	void UnloadAllModdedPaks();

	// Tracks all Paks related to a mod.
	void TrackModPaks(Mod& mod);

	// Tracks a Pak that vanilla attempted to load.
	void TrackVanillaPak(const char* originalPath, int pakHandle);

	// Untracks all paks that aren't currently loaded and are marked for unload.
	void CleanUpUnloadedPaks();

	// Unloads all paks that are marked for unload.
	void UnloadMarkedPaks();

	// Loads all modded paks for the given map.
	void LoadMapPaks(const char* mapName);
	// Unloads all modded map paks
	void UnloadMapPaks();

private:
	// Loads Paks that depend on this Pak.
	void LoadDependentPaks(const char* pakPath);

	// Finds a Pak by hash.
	ModPak* FindPak(size_t pathHash);

	// All paks that vanilla has attempted to load. (they may have been aliased away)
	// Also known as a list of rpaks that the vanilla game would have loaded at this point in time.
	std::vector<std::pair<std::string, int>> m_vanillaPaks;

	// All mod Paks
	std::vector<ModPak> m_modPaks;

	// Currently loaded map mod paks
	std::vector<size_t> m_mapPaks;

	// todo: deprecate these?
	// Paks that are always loaded (Preload)
	std::vector<size_t> m_constantPaks;
	// Paks that are dependent on another Pak being loaded (Postload)
	std::vector<std::pair<size_t, size_t>> m_dependentPaks;
	// Paks that fully replace (Alias) a target Pak
	std::vector<std::pair<size_t, size_t>> m_aliasPaks;

	bool m_vanillaCall = true;
};

extern PakLoadManager* g_pPakLoadManager;
extern NewPakLoadManager* g_pNewPakLoadManager;
