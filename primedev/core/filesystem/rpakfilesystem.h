#pragma once

#include <regex>

struct ModPak
{
	std::string m_modName;
	std::string m_path;
	size_t m_pathHash;

	std::regex m_mapRegex;
	size_t m_dependentPakHash;
	bool m_preload = false;

	// if this is set, the Pak will be unloaded on next map load
	bool m_markedForDelete = false;
	int m_handle = -1;
};

/*
* [X] on mod reload, mark all paks for unloading
* [X] on mod load, read rpak.json and track rpaks
* [X] on map change, load the correct mod rpaks
* [X] on map change, unload paks that are marked for unloading
* [X] on pak load, add to vanilla tracked paks (if static bool is true)
* [ ] on pak load, load dependent rpaks after
* [X] on pak unload, unload dependent rpaks first
* [ ] on pak unload, if pak was aliasing a vanilla pak away, load the vanilla pak (if static bool is false)
*/

class NewPakLoadManager
{
public:
	// Marks all mod Paks to be unloaded on next map load.
	// Also cleans up any mod Paks that are already unloaded.
	void UnloadAllModPaks();

	// Tracks all Paks related to a mod.
	void TrackModPaks(Mod& mod);

	// Untracks all paks that aren't currently loaded and are marked for unload.
	void CleanUpUnloadedPaks();

	// Unloads all paks that are marked for unload.
	void UnloadMarkedPaks();

	// Loads all modded paks for the given map.
	void LoadModPaksForMap(const char* mapName);
	// Unloads all modded map paks.
	void UnloadModPaks();

	// Whether the current context is a vanilla call to a function, or a modded one
	bool IsVanillaCall() const { return m_reentranceCounter == 0; }
	// Whether paks will be forced to reload on the next map load
	bool GetForceReloadOnMapLoad() const { return m_forceReloadOnMapLoad; }
	void SetForceReloadOnMapLoad(bool value) { m_forceReloadOnMapLoad = value; }

	// Called after a Pak was loaded.
	void OnPakLoaded(std::string& originalPath, std::string& resultingPath, int resultingHandle);
	// Called before a Pak was unloaded.
	void OnPakUnloading(int handle);

	// If vanilla doesn't have an rpak for this path, tries to map it to a modded rpak of the same name.
	void FixupPakPath(std::string& path);

	// Loads all "Preload" Paks. todo: deprecate.
	void LoadPreloadPaks();

	// Wrapper for Pak load API.
	void* OpenFile(const char* path);

private:
	// Loads Paks that depend on this Pak.
	void LoadDependentPaks(std::string& path, int handle);
	// Unloads Paks that depend on this Pak.
	void UnloadDependentPaks(int handle);

	// All paks that vanilla has attempted to load. (they may have been aliased away)
	// Also known as a list of rpaks that the vanilla game would have loaded at this point in time.
	std::vector<std::pair<std::string, int>> m_vanillaPaks;

	// All mod Paks that are currently tracked
	std::vector<ModPak> m_modPaks;
	// Hashes of the currently loaded map mod paks
	std::vector<size_t> m_mapPaks;
	// Currently loaded Pak path hashes that depend on a handle to remain loaded (Postload)
	std::vector<std::pair<int, size_t>> m_dependentPaks;

	// Used to force rpaks to be unloaded and reloaded on the next map load.
	// Vanilla behaviour is to not do this when loading into mp_lobby, or loading into the same map you were last in.
	bool m_forceReloadOnMapLoad = false;
	// Used to track if the current hook call is a vanilla call or not.
	// When loading/unloading a mod Pak, increment this before doing so, and decrement afterwards.
	int m_reentranceCounter = 0;
};

extern NewPakLoadManager* g_pNewPakLoadManager;
