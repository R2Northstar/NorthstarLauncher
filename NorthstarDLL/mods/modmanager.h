#pragma once
#include "core/convar/convar.h"
#include "core/memalloc.h"
#include "squirrel/squirrel.h"

#include "rapidjson/document.h"
#include <string>
#include <vector>
#include <filesystem>
#include <unordered_set>

const std::string MOD_FOLDER_SUFFIX = "mods";
const std::string REMOTE_MOD_FOLDER_SUFFIX = "runtime/remote/mods";
const fs::path MOD_OVERRIDE_DIR = "mod";
const std::string COMPILED_ASSETS_SUFFIX = "runtime/compiled";

struct ModConVar
{
	std::string Name;
	std::string DefaultValue;
	std::string HelpString;
	int Flags;
};

struct ModConCommand
{
	std::string Name;
	std::string Function;
	std::string HelpString;
	ScriptContext Context;
	int Flags;
};

struct ModScriptCallback
{
	ScriptContext Context;

	// called before the codecallback is executed
	std::string BeforeCallback;
	// called after the codecallback has finished executing
	std::string AfterCallback;
	// called right before the vm is destroyed.
	std::string DestroyCallback;
};

struct ModScript
{
	std::string Path;
	std::string RunOn;

	std::vector<ModScriptCallback> Callbacks;
};

// these are pretty much identical, could refactor to use the same stuff?
struct ModVPKEntry
{
	bool m_bAutoLoad;
	std::string m_sVpkPath;
};

struct ModRpakEntry
{
	bool m_bAutoLoad;
	std::string m_sPakName;
	std::string m_sLoadAfterPak;
};

struct Mod
{
	// runtime stuff
	bool m_bEnabled = true;
	bool m_bWasReadSuccessfully = false;
	fs::path m_ModDirectory;
	bool m_bRemote; // whether the mod was automatically downloaded

	// mod.json stuff:

	// the mod's name
	std::string Name;
	// the mod's description
	std::string Description;
	// the mod's version, should be in semver
	std::string Version;
	// a download link to the mod, for clients that try to join without the mod
	std::string DownloadLink;

	// whether clients need the mod to join servers running this mod
	bool RequiredOnClient;
	// the priority for this mod's files, mods with prio 0 are loaded first, then 1, then 2, etc
	int LoadPriority;

	// custom scripts used by the mod
	std::vector<ModScript> Scripts;
	// convars created by the mod
	std::vector<ModConVar> ConVars;
	// concommands created by the mod
	std::vector<ModConCommand> ConCommands;
	// custom localisation files created by the mod
	std::vector<std::string> LocalisationFiles;

	// other files:

	std::vector<ModVPKEntry> Vpks;
	std::unordered_map<size_t, std::string> KeyValues;
	std::vector<std::string> BinkVideos;

	// todo audio override struct

	std::vector<ModRpakEntry> Rpaks;
	std::unordered_map<std::string, std::string>
		RpakAliases; // paks we alias to other rpaks, e.g. to load sp_crashsite paks on the map mp_crashsite
	std::vector<size_t> StarpakPaths; // starpaks that this mod contains
	// there seems to be no nice way to get the rpak that is causing the load of a starpak?
	// hashed with STR_HASH

	std::unordered_map<std::string, std::string> DependencyConstants;

  public:
	Mod(fs::path modPath, std::string sJson, bool bRemote);
};

struct ModOverrideFile
{
	Mod* m_pOwningMod; // don't need to explicitly clean this up
	fs::path m_Path;
	fs::file_time_type m_tLastWriteTime;
};

class ModManager
{
  private:
	bool m_bHasLoadedMods = false;
	bool m_bHasEnabledModsCfg;

	// precalculated hashes
	size_t m_hScriptsRsonHash;
	size_t m_hPdefHash;
	size_t m_hKBActHash;

	void LoadModDefinitions();
	void SaveEnabledMods();
	void InstallMods();

	// mod installation funcs
	void InstallModCvars(Mod& mod);
	void InstallModVpks(Mod& mod);
	void InstallModRpaks(Mod& mod);
	void InstallModKeyValues(Mod& mod);
	void InstallModBinks(Mod& mod);
	void InstallModAudioOverrides(Mod& mod);

	void InstallModFileOverrides(Mod& mod);

	void UnloadMods();

	std::unordered_set<ConVar*> m_RegisteredModConVars;
	std::unordered_set<ConCommand*> m_RegisteredModConCommands;


	struct
	{
		// assets types we need to reload completely after mods are reloaded (can't reload individually)
		bool bUiScript = false; // also includes .menu files
		bool bLocalisation = false;
		bool bPlaylists = false;
		bool bAimAssistSettings = false;
		bool bMaterials = false; // vmts
		bool bWeaponSettings = false;
		bool bPlayerSettings = false;
		bool bAiSettings = false;
		bool bDamageDefs = false; // damagedefs
		bool bDatatables = false;

		// can't actually reload this atm, just print a warning (todo, could maybe restart client to ensure loaded?)
		bool bModels = false;

		bool bRPaks = false;

		// assets that we can reload individually
		//std::vector<ModAudioOverride> vAudioOverrides
	} m_AssetTypesToReload;

	void CheckModFilesForChanges();
	void ReloadNecessaryModAssets();


	struct ModLoadState
	{
		std::vector<Mod> m_LoadedMods;
		std::unordered_map<std::string, ModOverrideFile> m_ModFiles;
		std::unordered_map<std::string, std::string> m_DependencyConstants;
	};

	// unfortunately need to be ptrs, so we can copy m_ModLoadState => m_LastModLoadState
	// without ptrs to any elements (only ModOverrideFile::m_pOwningMod atm) decaying
	// unfortunately means we need to manually manage memory of them, but oh well
	ModLoadState* m_LastModLoadState;
	ModLoadState* m_ModLoadState;

  public:
	ModManager();
	void LoadMods();
	std::string NormaliseModFilePath(const fs::path path);
	void CompileAssetsForFile(const char* filename);

	// getters
	inline std::vector<Mod>& GetMods()
	{
		return m_ModLoadState->m_LoadedMods;
	};

	inline std::unordered_map<std::string, ModOverrideFile>& GetModFiles()
	{
		return m_ModLoadState->m_ModFiles;
	};

	inline std::unordered_map<std::string, std::string>& GetDependencyConstants()
	{
		return m_ModLoadState->m_DependencyConstants;
	};

	// compile asset type stuff, these are done in files under runtime/compiled/
	void BuildScriptsRson();
	void TryBuildKeyValues(const char* filename);
	void BuildKBActionsList();

	// for std::views::filter, e.g. for (Mod& mod : g_pModManager::GetMods() | ModManager::FilterEnabled)
	static inline constexpr auto FilterEnabled = std::views::filter([](Mod& mod) { return mod.m_bEnabled; });
	static inline constexpr auto FilterRemote = std::views::filter([](Mod& mod) { return mod.m_bRemote; });
	static inline constexpr auto FilterLocal = std::views::filter([](Mod& mod) { return !mod.m_bRemote; });
};

fs::path GetModFolderPath();
fs::path GetRemoteModFolderPath();
fs::path GetCompiledAssetsPath();

extern ModManager* g_pModManager;
