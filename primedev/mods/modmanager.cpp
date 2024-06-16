#include "modmanager.h"
#include "core/convar/convar.h"
#include "core/convar/concommand.h"
#include "client/audio.h"
#include "masterserver/masterserver.h"
#include "core/filesystem/filesystem.h"
#include "core/filesystem/rpakfilesystem.h"
#include "core/json.h"
#include "config/profile.h"

#include "yyjson.h"
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <regex>

ModManager* g_pModManager;

Mod::Mod(fs::path modDir, char* jsonBuf)
{
	m_bWasReadSuccessfully = false;

	m_ModDirectory = modDir;

	yyjson_read_flag flg = YYJSON_READ_ALLOW_COMMENTS | YYJSON_READ_ALLOW_TRAILING_COMMAS;
	yyjson_read_err err;
	yyjson_doc* doc = yyjson_read_opts(jsonBuf, strlen(jsonBuf), flg, &YYJSON_ALLOCATOR, &err);

	spdlog::info("Loading mod file at path '{}'", modDir.string());

	// fail if parse error
	if (!doc)
	{
		spdlog::error(
			"Failed reading mod file {}: encountered parse error \"{}\" at offset {}",
			(modDir / "mod.json").string(),
			err.msg,
			err.pos);
		return;
	}

	yyjson_val* root = yyjson_doc_get_root(doc);

	// fail if it's not a json obj (could be an array, string, etc)
	if (!yyjson_is_obj(root))
	{
		spdlog::error("Failed reading mod file {}: file is not a JSON object", (modDir / "mod.json").string());
		return;
	}

	yyjson_val* nameVal = yyjson_obj_get(root, "Name");

	// basic mod info
	// name is required
	if (!nameVal)
	{
		spdlog::error("Failed reading mod file {}: missing required member \"Name\"", (modDir / "mod.json").string());
		return;
	}

	Name = yyjson_get_str(nameVal);
	spdlog::info("Loading mod '{}'", Name);

	// Don't load blacklisted mods
	if (!strstr(GetCommandLineA(), "-nomodblacklist") && MODS_BLACKLIST.find(Name) != std::end(MODS_BLACKLIST))
	{
		spdlog::warn("Skipping blacklisted mod \"{}\"!", Name);
		return;
	}

	yyjson_val* descriptionVal = yyjson_obj_get(root, "Description");
	if (descriptionVal)
		Description = yyjson_get_str(descriptionVal);
	else
		Description = "";

	yyjson_val* versionVal = yyjson_obj_get(root, "Version");
	if (versionVal)
		Version = yyjson_get_str(versionVal);
	else
	{
		Version = "0.0.0";
		spdlog::warn("Mod file {} is missing a version, consider adding a version", (modDir / "mod.json").string());
	}

	yyjson_val* downloadVal = yyjson_obj_get(root, "DownloadLink");
	if (downloadVal)
		DownloadLink = yyjson_get_str(downloadVal);
	else
		DownloadLink = "";

	yyjson_val* requiredVal = yyjson_obj_get(root, "RequiredOnClient");
	if (requiredVal)
		RequiredOnClient = yyjson_get_bool(requiredVal);
	else
		RequiredOnClient = false;

	yyjson_val* priorityVal = yyjson_obj_get(root, "LoadPriority");
	if (priorityVal)
		LoadPriority = yyjson_get_int(priorityVal);
	else
	{
		spdlog::info("Mod file {} is missing a LoadPriority, consider adding one", (modDir / "mod.json").string());
		LoadPriority = 0;
	}

	// Parse all array fields
	ParseConVars(root);
	ParseConCommands(root);
	ParseScripts(root);
	ParseLocalization(root);
	ParseDependencies(root);
	ParsePluginDependencies(root);
	ParseInitScript(root);

	// A mod is remote if it's located in the remote mods folder
	m_bIsRemote = m_ModDirectory.generic_string().find(GetRemoteModFolderPath().generic_string()) != std::string::npos;

	m_bWasReadSuccessfully = true;
}

void Mod::ParseConVars(yyjson_val* json)
{
	yyjson_val* conVarsVal = yyjson_obj_get(json, "ConVars");
	if (!conVarsVal)
		return;

	if (!yyjson_is_arr(conVarsVal))
	{
		spdlog::warn("'ConVars' field is not an array, skipping...");
		return;
	}

	size_t idx, max;
	yyjson_val* val;
	yyjson_arr_foreach(conVarsVal, idx, max, val)
	{
		if (!yyjson_is_obj(val))
		{
			spdlog::warn("ConVar is not an object, skipping...");
			continue;
		}

		const char* name = yyjson_get_str(yyjson_obj_get(val, "Name"));
		if (!name)
		{
			spdlog::warn("ConVar does not have a Name, skipping...");
			continue;
		}

		// from here on, the ConVar can be referenced by name in logs
		const char* defaultValue = yyjson_get_str(yyjson_obj_get(val, "DefaultValue"));
		if (!defaultValue)
		{
			spdlog::warn("ConVar '{}' does not have a DefaultValue, skipping...", name);
			continue;
		}

		// have to allocate this manually, otherwise convar registration will break
		// unfortunately this causes us to leak memory on reload, unsure of a way around this rn
		ModConVar* convar = new ModConVar;
		convar->Name = name;
		convar->DefaultValue = defaultValue;

		const char* helpString = yyjson_get_str(yyjson_obj_get(val, "HelpString"));
		if (helpString)
			convar->HelpString = helpString;
		else
			convar->HelpString = "";

		convar->Flags = FCVAR_NONE;

		yyjson_val* flags = yyjson_obj_get(val, "Flags");
		if (flags)
		{
			// read raw integer flags
			if (yyjson_is_int(flags))
				convar->Flags = yyjson_get_int(flags);
			else if (yyjson_is_str(flags))
			{
				// parse cvar flags from string
				// example string: ARCHIVE_PLAYERPROFILE | GAMEDLL
				convar->Flags |= ParseConVarFlagsString(convar->Name, yyjson_get_str(flags));
			}
		}

		ConVars.push_back(convar);

		spdlog::info("'{}' contains ConVar '{}'", Name, convar->Name);
	}
}

void Mod::ParseConCommands(yyjson_val* json)
{
	yyjson_val* conCommandsVal = yyjson_obj_get(json, "ConCommands");
	if (!conCommandsVal)
		return;

	if (!yyjson_is_arr(conCommandsVal))
	{
		spdlog::warn("'ConCommands' field is not an array, skipping...");
		return;
	}

	size_t idx, max;
	yyjson_val* val;
	yyjson_arr_foreach(conCommandsVal, idx, max, val)
	{
		if (!yyjson_is_obj(val))
		{
			spdlog::warn("ConCommand is not an object, skipping...");
			continue;
		}

		const char* name = yyjson_get_str(yyjson_obj_get(val, "Name"));
		if (!name)
		{
			spdlog::warn("ConCommand does not have a Name, skipping...");
			continue;
		}

		// from here on, the ConCommand can be referenced by name in logs
		const char* function = yyjson_get_str(yyjson_obj_get(val, "Function"));
		if (!function)
		{
			spdlog::warn("ConCommand '{}' does not have a Function, skipping...", name);
			continue;
		}

		const char* context = yyjson_get_str(yyjson_obj_get(val, "Context"));
		if (!context)
		{
			spdlog::warn("ConCommand '{}' does not have a Context, skipping...", name);
			continue;
		}

		// have to allocate this manually, otherwise concommand registration will break
		// unfortunately this causes us to leak memory on reload, unsure of a way around this rn
		ModConCommand* concommand = new ModConCommand;
		concommand->Name = name;
		concommand->Function = function;
		concommand->Context = ScriptContextFromString(context);
		if (concommand->Context == ScriptContext::INVALID)
		{
			spdlog::warn("ConCommand '{}' has invalid context '{}', skipping...", concommand->Name, context);
			continue;
		}

		const char* helpString = yyjson_get_str(yyjson_obj_get(val, "HelpString"));
		if (helpString)
			concommand->HelpString = helpString;
		else
			concommand->HelpString = "";

		concommand->Flags = FCVAR_NONE;

		yyjson_val* flags = yyjson_obj_get(val, "Flags");
		if (flags)
		{
			// read raw integer flags
			if (yyjson_is_int(flags))
			{
				concommand->Flags = yyjson_get_int(flags);
			}
			else if (yyjson_is_str(flags))
			{
				// parse cvar flags from string
				// example string: ARCHIVE_PLAYERPROFILE | GAMEDLL
				concommand->Flags |= ParseConVarFlagsString(concommand->Name, yyjson_get_str(flags));
			}
		}

		ConCommands.push_back(concommand);

		spdlog::info("'{}' contains ConCommand '{}'", Name, concommand->Name);
	}
}

void Mod::ParseScripts(yyjson_val* json)
{
	yyjson_val* scriptsVal = yyjson_obj_get(json, "Scripts");
	if (!scriptsVal)
		return;

	if (!yyjson_is_arr(scriptsVal))
	{
		spdlog::warn("'Scripts' field is not an array, skipping...");
		return;
	}

	size_t idx, max;
	yyjson_val* val;
	yyjson_arr_foreach(scriptsVal, idx, max, val)
	{
		if (!yyjson_is_obj(val))
		{
			spdlog::warn("Script is not an object, skipping...");
			continue;
		}
		
		const char* path = yyjson_get_str(yyjson_obj_get(val, "Path"));
		if (!path)
		{
			spdlog::warn("Script does not have a Path, skipping...");
			continue;
		}

		// from here on, the Path for a script is used as it's name in logs
		const char* runOn = yyjson_get_str(yyjson_obj_get(val, "RunOn"));
		if (!runOn)
		{
			// "a RunOn" sounds dumb but anything else doesn't match the format of the warnings...
			// this is the best i could think of within 20 seconds
			spdlog::warn("Script '{}' does not have a RunOn field, skipping...", path);
			continue;
		}

		ModScript script;

		script.Path = path;
		script.RunOn = runOn;

		yyjson_val* serverCallback = yyjson_obj_get(val, "ServerCallback");
		if (serverCallback)
		{
			if (yyjson_is_obj(serverCallback))
			{
				ModScriptCallback callback;
				callback.Context = ScriptContext::SERVER;

				yyjson_val* callbackBefore = yyjson_obj_get(serverCallback, "Before");
				if (callbackBefore)
				{
					if (yyjson_is_str(callbackBefore))
						callback.BeforeCallback = yyjson_get_str(callbackBefore);
					else
						spdlog::warn("'Before' ServerCallback for script '{}' is not a string, skipping...", path);
				}

				yyjson_val* callbackAfter = yyjson_obj_get(serverCallback, "After");
				if (callbackAfter)
				{
					if (yyjson_is_str(callbackAfter))
						callback.AfterCallback = yyjson_get_str(callbackAfter);
					else
						spdlog::warn("'After' ServerCallback for script '{}' is not a string, skipping...", path);
				}

				yyjson_val* callbackDestroy = yyjson_obj_get(serverCallback, "Destroy");
				if (callbackDestroy)
				{
					if (yyjson_is_str(callbackDestroy))
						callback.DestroyCallback = yyjson_get_str(callbackDestroy);
					else
						spdlog::warn(
							"'Destroy' ServerCallback for script '{}' is not a string, skipping...", path);
				}

				script.Callbacks.push_back(callback);
			}
			else
			{
				spdlog::warn("ServerCallback for script '{}' is not an object, skipping...", path);
			}
		}

		yyjson_val* clientCallback = yyjson_obj_get(val, "ClientCallback");
		if (clientCallback)
		{
			if (yyjson_is_obj(clientCallback))
			{
				ModScriptCallback callback;
				callback.Context = ScriptContext::CLIENT;

				yyjson_val* callbackBefore = yyjson_obj_get(clientCallback, "Before");
				if (callbackBefore)
				{
					if (yyjson_is_str(callbackBefore))
						callback.BeforeCallback = yyjson_get_str(callbackBefore);
					else
						spdlog::warn("'Before' ClientCallback for script '{}' is not a string, skipping...", path);
				}

				yyjson_val* callbackAfter = yyjson_obj_get(clientCallback, "After");
				if (callbackAfter)
				{
					if (yyjson_is_str(callbackAfter))
						callback.AfterCallback = yyjson_get_str(callbackAfter);
					else
						spdlog::warn("'After' ClientCallback for script '{}' is not a string, skipping...", path);
				}

				yyjson_val* callbackDestroy = yyjson_obj_get(clientCallback, "Destroy");
				if (callbackDestroy)
				{
					if (yyjson_is_str(callbackDestroy))
						callback.DestroyCallback = yyjson_get_str(callbackDestroy);
					else
						spdlog::warn(
							"'Destroy' ClientCallback for script '{}' is not a string, skipping...", path);
				}

				script.Callbacks.push_back(callback);
			}
			else
			{
				spdlog::warn("ClientCallback for script '{}' is not an object, skipping...", path);
			}
		}

		yyjson_val* uiCallback = yyjson_obj_get(val, "UICallback");
		if (uiCallback)
		{
			if (yyjson_is_obj(uiCallback))
			{
				ModScriptCallback callback;
				callback.Context = ScriptContext::UI;

				yyjson_val* callbackBefore = yyjson_obj_get(uiCallback, "Before");
				if (callbackBefore)
				{
					if (yyjson_is_str(callbackBefore))
						callback.BeforeCallback = yyjson_get_str(callbackBefore);
					else
						spdlog::warn("'Before' UICallback for script '{}' is not a string, skipping...", path);
				}

				yyjson_val* callbackAfter = yyjson_obj_get(uiCallback, "After");
				if (callbackAfter)
				{
					if (yyjson_is_str(callbackAfter))
						callback.AfterCallback = yyjson_get_str(callbackAfter);
					else
						spdlog::warn("'After' UICallback for script '{}' is not a string, skipping...", path);
				}

				yyjson_val* callbackDestroy = yyjson_obj_get(uiCallback, "Destroy");
				if (callbackDestroy)
				{
					if (yyjson_is_str(callbackDestroy))
						callback.DestroyCallback = yyjson_get_str(callbackDestroy);
					else
						spdlog::warn("'Destroy' UICallback for script '{}' is not a string, skipping...", path);
				}

				script.Callbacks.push_back(callback);
			}
			else
			{
				spdlog::warn("UICallback for script '{}' is not an object, skipping...", path);
			}
		}

		Scripts.push_back(script);

		spdlog::info("'{}' contains Script '{}'", Name, script.Path);
	}
}

void Mod::ParseLocalization(yyjson_val* json)
{
	yyjson_val* localisationVal = yyjson_obj_get(json, "Localisation");
	if (!localisationVal)
		return;

	if (!yyjson_is_arr(localisationVal))
	{
		spdlog::warn("'Localisation' field is not an array, skipping...");
		return;
	}

	size_t idx, max;
	yyjson_val* val;
	yyjson_arr_foreach(localisationVal, idx, max, val)
	{
		if (!yyjson_is_str(val))
		{
			// not a string but we still GetString() to log it :trol:
			spdlog::warn("Localisation is not a string, skipping...");
			continue;
		}

		const char* localisationStr = yyjson_get_str(val);

		LocalisationFiles.push_back(localisationStr);

		spdlog::info("'{}' registered Localisation '{}'", Name, localisationStr);
	}
}

void Mod::ParseDependencies(yyjson_val* json)
{
	yyjson_val* dependenciesVal = yyjson_obj_get(json, "Dependencies");
	if (!dependenciesVal)
		return;

	if (!yyjson_is_obj(dependenciesVal))
	{
		spdlog::warn("'Dependencies' field is not an object, skipping...");
		return;
	}

	size_t idx, max;
	yyjson_val* key;
	yyjson_val* val;
	yyjson_obj_foreach(dependenciesVal, idx, max, key, val)
	{
		if (!yyjson_is_str(key))
		{
			spdlog::warn("Dependency constant key is not a string, skipping...");
			continue;
		}

		std::string key_name = yyjson_get_str(key);
		if (!yyjson_is_str(val))
		{
			spdlog::warn("Dependency constant value for '{}' is not a string, skipping...", key_name);
			continue;
		}

		std::string val_name = yyjson_get_str(val);
		if (DependencyConstants.find(key_name) != DependencyConstants.end() &&
			val_name != DependencyConstants[key_name])
		{
			// this is fatal because otherwise the mod will probably try to use functions that dont exist,
			// which will cause errors further down the line that are harder to debug
			spdlog::error(
				"'{}' attempted to register a dependency constant '{}' for '{}' that already exists for '{}'. "
				"Change the constant name.",
				Name,
				key_name,
				val_name,
				DependencyConstants[key_name]);
			return;
		}

		if (DependencyConstants.find(key_name) == DependencyConstants.end())
			DependencyConstants.emplace(key_name, val_name);

		spdlog::info("'{}' registered dependency constant '{}' for mod '{}'", Name, key_name, val_name);
	}
}

void Mod::ParsePluginDependencies(yyjson_val* json)
{
	yyjson_val* pluginDependenciesVal = yyjson_obj_get(json, "PluginDependencies");
	if (!pluginDependenciesVal)
		return;

	if (!yyjson_is_arr(pluginDependenciesVal))
	{
		spdlog::warn("'PluginDependencies' field is not an array, skipping...");
		return;
	}

	size_t idx, max;
	yyjson_val* val;
	yyjson_arr_foreach(pluginDependenciesVal, idx, max, val)
	{
		if (!yyjson_is_str(val))
			continue;

		const char* name = yyjson_get_str(val);

		spdlog::info("Plugin Constant {} defined by {}", name, Name);

		PluginDependencyConstants.push_back(name);
	}
}

void Mod::ParseInitScript(yyjson_val* json)
{
	yyjson_val* initScriptVal = yyjson_obj_get(json, "InitScript");
	if (!initScriptVal)
		return;

	if (!yyjson_is_str(initScriptVal))
	{
		spdlog::warn("'InitScript' field is not a string, skipping...");
		return;
	}

	initScript = yyjson_get_str(initScriptVal);
}

ModManager::ModManager()
{
	// precaculated string hashes
	// note: use backslashes for these, since we use lexically_normal for file paths which uses them
	m_hScriptsRsonHash = STR_HASH("scripts\\vscripts\\scripts.rson");
	m_hPdefHash = STR_HASH(
		"cfg\\server\\persistent_player_data_version_231.pdef" // this can have multiple versions, but we use 231 so that's what we hash
	);
	m_hKBActHash = STR_HASH("scripts\\kb_act.lst");

	LoadMods();
}

ModManager::~ModManager()
{
	if (m_EnabledModsCfg)
	{
		yyjson_mut_doc_free(m_EnabledModsCfg);
	}
}

struct Test
{
	std::string funcName;
	ScriptContext context;
};

template <ScriptContext context> auto ModConCommandCallback_Internal(std::string name, const CCommand& command)
{
	if (g_pSquirrel<context>->m_pSQVM && g_pSquirrel<context>->m_pSQVM)
	{
		if (command.ArgC() == 1)
		{
			g_pSquirrel<context>->AsyncCall(name);
		}
		else
		{
			std::vector<std::string> args;
			args.reserve(command.ArgC());
			for (int i = 1; i < command.ArgC(); i++)
				args.push_back(command.Arg(i));
			g_pSquirrel<context>->AsyncCall(name, args);
		}
	}
	else
	{
		spdlog::warn("ConCommand `{}` was called while the associated Squirrel VM `{}` was unloaded", name, GetContextName(context));
	}
}

auto ModConCommandCallback(const CCommand& command)
{
	ModConCommand* found = nullptr;
	auto commandString = std::string(command.GetCommandString());

	// Finding the first space to remove the command's name
	auto firstSpace = commandString.find(' ');
	if (firstSpace)
	{
		commandString = commandString.substr(0, firstSpace);
	}

	// Find the mod this command belongs to
	for (auto& mod : g_pModManager->m_LoadedMods)
	{
		auto res = std::find_if(
			mod.ConCommands.begin(),
			mod.ConCommands.end(),
			[&commandString](const ModConCommand* other) { return other->Name == commandString; });
		if (res != mod.ConCommands.end())
		{
			found = *res;
			break;
		}
	}
	if (!found)
		return;

	switch (found->Context)
	{
	case ScriptContext::CLIENT:
		ModConCommandCallback_Internal<ScriptContext::CLIENT>(found->Function, command);
		break;
	case ScriptContext::SERVER:
		ModConCommandCallback_Internal<ScriptContext::SERVER>(found->Function, command);
		break;
	case ScriptContext::UI:
		ModConCommandCallback_Internal<ScriptContext::UI>(found->Function, command);
		break;
	default:
		spdlog::error("ModConCommandCallback on invalid Context {}", found->Context);
	};
}

void ModManager::LoadMods()
{
	if (m_bHasLoadedMods)
		UnloadMods();

	std::vector<fs::path> modDirs;

	// ensure dirs exist
	fs::remove_all(GetCompiledAssetsPath());
	fs::create_directories(GetModFolderPath());
	fs::create_directories(GetThunderstoreModFolderPath());
	fs::create_directories(GetRemoteModFolderPath());

	m_DependencyConstants.clear();

	// read enabled mods cfg
	std::ifstream enabledModsStream(GetNorthstarPrefix() + "/enabledmods.json");
	std::stringstream enabledModsStringStream;

	if (!enabledModsStream.fail())
	{
		while (enabledModsStream.peek() != EOF)
			enabledModsStringStream << (char)enabledModsStream.get();

		std::string enabledModsString = enabledModsStringStream.str();

		enabledModsStream.close();

		yyjson_read_flag flg = YYJSON_READ_ALLOW_COMMENTS | YYJSON_READ_ALLOW_TRAILING_COMMAS;
		if (m_EnabledModsCfg)
		{
			yyjson_mut_doc_free(m_EnabledModsCfg);
		}

		yyjson_doc* doc = yyjson_read_opts(const_cast<char*>(enabledModsString.c_str()), enabledModsString.length(), flg, &YYJSON_ALLOCATOR, NULL);

		m_EnabledModsCfg = yyjson_doc_mut_copy(doc, &YYJSON_ALLOCATOR);

		yyjson_doc_free(doc);

		yyjson_mut_val* root = yyjson_mut_doc_get_root(m_EnabledModsCfg);

		m_bHasEnabledModsCfg = yyjson_mut_is_obj(root);
	}

	// get mod directories
	std::filesystem::directory_iterator classicModsDir = fs::directory_iterator(GetModFolderPath());
	std::filesystem::directory_iterator remoteModsDir = fs::directory_iterator(GetRemoteModFolderPath());
	std::filesystem::directory_iterator thunderstoreModsDir = fs::directory_iterator(GetThunderstoreModFolderPath());

	for (fs::directory_entry dir : classicModsDir)
		if (fs::exists(dir.path() / "mod.json"))
			modDirs.push_back(dir.path());

	// Special case for Thunderstore and remote mods directories
	// Set up regex for `AUTHOR-MOD-VERSION` pattern
	std::regex pattern(R"(.*\\([a-zA-Z0-9_]+)-([a-zA-Z0-9_]+)-(\d+\.\d+\.\d+))");

	for (fs::directory_iterator dirIterator : {thunderstoreModsDir, remoteModsDir})
	{
		for (fs::directory_entry dir : dirIterator)
		{
			fs::path modsDir = dir.path() / "mods"; // Check for mods folder in the Thunderstore mod
			// Use regex to match `AUTHOR-MOD-VERSION` pattern
			if (!std::regex_match(dir.path().string(), pattern))
			{
				spdlog::warn("The following directory did not match 'AUTHOR-MOD-VERSION': {}", dir.path().string());
				continue; // skip loading mod that doesn't match
			}
			if (fs::exists(modsDir) && fs::is_directory(modsDir))
			{
				for (fs::directory_entry subDir : fs::directory_iterator(modsDir))
				{
					if (fs::exists(subDir.path() / "mod.json"))
					{
						modDirs.push_back(subDir.path());
					}
				}
			}
		}
	}

	for (fs::path modDir : modDirs)
	{
		// read mod json file
		std::ifstream jsonStream(modDir / "mod.json");
		std::stringstream jsonStringStream;

		// fail if no mod json
		if (jsonStream.fail())
		{
			spdlog::warn(
				"Mod file at '{}' does not exist or could not be read, is it installed correctly?", (modDir / "mod.json").string());
			continue;
		}

		while (jsonStream.peek() != EOF)
			jsonStringStream << (char)jsonStream.get();

		jsonStream.close();

		Mod mod(modDir, (char*)jsonStringStream.str().c_str());

		for (auto& pair : mod.DependencyConstants)
		{
			if (m_DependencyConstants.find(pair.first) != m_DependencyConstants.end() && m_DependencyConstants[pair.first] != pair.second)
			{
				spdlog::error(
					"'{}' attempted to register a dependency constant '{}' for '{}' that already exists for '{}'. "
					"Change the constant name.",
					mod.Name,
					pair.first,
					pair.second,
					m_DependencyConstants[pair.first]);
				mod.m_bWasReadSuccessfully = false;
				break;
			}
			if (m_DependencyConstants.find(pair.first) == m_DependencyConstants.end())
				m_DependencyConstants.emplace(pair);
		}

		for (std::string& dependency : mod.PluginDependencyConstants)
		{
			m_PluginDependencyConstants.insert(dependency);
		}

		yyjson_mut_val* root = yyjson_mut_doc_get_root(m_EnabledModsCfg);
		yyjson_mut_val* hasMod = yyjson_mut_obj_get(root, mod.Name.c_str());

		if (m_bHasEnabledModsCfg && hasMod)
			mod.m_bEnabled = yyjson_mut_is_true(hasMod);
		else
			mod.m_bEnabled = true;

		if (mod.m_bWasReadSuccessfully)
		{
			if (mod.m_bEnabled)
				spdlog::info("'{}' loaded successfully, version {}", mod.Name, mod.Version);
			else
				spdlog::info("'{}' loaded successfully, version {} (DISABLED)", mod.Name, mod.Version);

			m_LoadedMods.push_back(mod);
		}
		else
			spdlog::warn("Mod file at '{}' failed to load", (modDir / "mod.json").string());
	}

	// sort by load prio, lowest-highest
	std::sort(m_LoadedMods.begin(), m_LoadedMods.end(), [](Mod& a, Mod& b) { return a.LoadPriority < b.LoadPriority; });

	// This is used to check if some mods have a folder but no entry in enabledmods.json
	bool newModsDetected = false;

	for (Mod& mod : m_LoadedMods)
	{
		if (!mod.m_bEnabled)
			continue;

		yyjson_mut_val* root = yyjson_mut_doc_get_root(m_EnabledModsCfg);
		yyjson_mut_val* hasMod = yyjson_mut_obj_get(root, mod.Name.c_str());

		// Add mod entry to enabledmods.json if it doesn't exist
		if (!mod.m_bIsRemote && !hasMod)
		{
			yyjson_mut_obj_add_true(m_EnabledModsCfg, root, mod.Name.c_str());
			newModsDetected = true;
		}

		// register convars
		// for reloads, this is sorta barebones, when we have a good findconvar method, we could probably reset flags and stuff on
		// preexisting convars note: we don't delete convars if they already exist because they're used for script stuff, unfortunately this
		// causes us to leak memory on reload, but not much, potentially find a way to not do this at some point
		for (ModConVar* convar : mod.ConVars)
		{
			// make sure convar isn't registered yet, unsure if necessary but idk what
			// behaviour is for defining same convar multiple times
			if (!g_pCVar->FindVar(convar->Name.c_str()))
			{
				new ConVar(convar->Name.c_str(), convar->DefaultValue.c_str(), convar->Flags, convar->HelpString.c_str());
			}
		}

		for (ModConCommand* command : mod.ConCommands)
		{
			// make sure command isnt't registered multiple times.
			if (!g_pCVar->FindCommand(command->Name.c_str()))
			{
				std::string funcName = command->Function;
				RegisterConCommand(command->Name.c_str(), ModConCommandCallback, command->HelpString.c_str(), command->Flags);
			}
		}

		// read vpk paths
		if (fs::exists(mod.m_ModDirectory / "vpk"))
		{
			// read vpk cfg
			std::ifstream vpkJsonStream(mod.m_ModDirectory / "vpk/vpk.json");
			std::stringstream vpkJsonStringStream;

			bool bUseVPKJson = false;
			yyjson_doc* dVpkJson = nullptr;
			yyjson_val* root = nullptr;

			if (!vpkJsonStream.fail())
			{
				while (vpkJsonStream.peek() != EOF)
					vpkJsonStringStream << (char)vpkJsonStream.get();

				vpkJsonStream.close();

				std::string vpkJsonString = vpkJsonStringStream.str();

				yyjson_read_flag flg = YYJSON_READ_ALLOW_COMMENTS | YYJSON_READ_ALLOW_TRAILING_COMMAS;
				dVpkJson = yyjson_read_opts(const_cast<char*>(vpkJsonString.c_str()), vpkJsonString.length(), flg, &YYJSON_ALLOCATOR, NULL);
				root = yyjson_doc_get_root(dVpkJson);

				bUseVPKJson = yyjson_is_obj(root);
			}

			for (fs::directory_entry file : fs::directory_iterator(mod.m_ModDirectory / "vpk"))
			{
				// a bunch of checks to make sure we're only adding dir vpks and their paths are good
				// note: the game will literally only load vpks with the english prefix
				if (fs::is_regular_file(file) && file.path().extension() == ".vpk" &&
					file.path().string().find("english") != std::string::npos &&
					file.path().string().find(".bsp.pak000_dir") != std::string::npos)
				{
					std::string formattedPath = file.path().filename().string();

					// this really fucking sucks but it'll work
					std::string vpkName = formattedPath.substr(strlen("english"), formattedPath.find(".bsp") - 3);

					ModVPKEntry& modVpk = mod.Vpks.emplace_back();

					yyjson_val* preload = yyjson_obj_get(root, "Preload");
					yyjson_val* preloadVpk = yyjson_obj_get(preload, vpkName.c_str());

					modVpk.m_bAutoLoad = !bUseVPKJson || (yyjson_is_obj(preload) && yyjson_is_true(preloadVpk));
					modVpk.m_sVpkPath = (file.path().parent_path() / vpkName).string();

					if (m_bHasLoadedMods && modVpk.m_bAutoLoad)
						(*g_pFilesystem)->m_vtable->MountVPK(*g_pFilesystem, vpkName.c_str());
				}
			}
		}

		// read rpak paths
		if (fs::exists(mod.m_ModDirectory / "paks"))
		{
			// read rpak cfg
			std::ifstream rpakJsonStream(mod.m_ModDirectory / "paks/rpak.json");
			std::stringstream rpakJsonStringStream;

			bool bUseRpakJson = false;
			yyjson_doc* dRpakJson = nullptr;
			yyjson_val* root = nullptr;

			if (!rpakJsonStream.fail())
			{
				while (rpakJsonStream.peek() != EOF)
					rpakJsonStringStream << (char)rpakJsonStream.get();

				rpakJsonStream.close();

				std::string rpakJsonString = rpakJsonStringStream.str();

				yyjson_read_flag flg = YYJSON_READ_ALLOW_COMMENTS | YYJSON_READ_ALLOW_TRAILING_COMMAS;
				dRpakJson = yyjson_read_opts(const_cast<char*>(rpakJsonString.c_str()), rpakJsonString.length(), flg, &YYJSON_ALLOCATOR, 0);
				root = yyjson_doc_get_root(dRpakJson);

				bUseRpakJson = yyjson_is_obj(root);
			}

			// read pak aliases
			yyjson_val* aliases = yyjson_obj_get(root, "Aliases");
			if (bUseRpakJson && yyjson_is_obj(aliases))
			{
				size_t idx, max;
				yyjson_val* key;
				yyjson_val* val;
				yyjson_obj_foreach(aliases, idx, max, key, val)
				{
					if (!yyjson_is_str(key)|| !yyjson_is_str(val))
						continue;

					mod.RpakAliases.insert(std::make_pair(yyjson_get_str(key), yyjson_get_str(val)));
				}
			}

			for (fs::directory_entry file : fs::directory_iterator(mod.m_ModDirectory / "paks"))
			{
				// ensure we're only loading rpaks
				if (fs::is_regular_file(file) && file.path().extension() == ".rpak")
				{
					std::string pakName(file.path().filename().string());

					ModRpakEntry& modPak = mod.Rpaks.emplace_back();

					yyjson_val* preload = yyjson_obj_get(root, "Preload");
					yyjson_val* preloadPak = yyjson_obj_get(preload, pakName.c_str());

					modPak.m_bAutoLoad =
						!bUseRpakJson || (yyjson_is_obj(preload) && yyjson_is_true(preloadPak));

					yyjson_val* postload = yyjson_obj_get(root, "Postload");
					yyjson_val* postloadPak = yyjson_obj_get(postload, pakName.c_str());

					// postload things
					if (!bUseRpakJson || (yyjson_is_obj(postload) && yyjson_is_str(postloadPak)))
						modPak.m_sLoadAfterPak = yyjson_get_str(postloadPak);

					modPak.m_sPakName = pakName;

					// read header of file and get the starpak paths
					// this is done here as opposed to on starpak load because multiple rpaks can load a starpak
					// and there is seemingly no good way to tell which rpak is causing the load of a starpak :/

					std::ifstream rpakStream(file.path(), std::ios::binary);

					// seek to the point in the header where the starpak reference size is
					rpakStream.seekg(0x38, std::ios::beg);
					int starpaksSize = 0;
					rpakStream.read((char*)&starpaksSize, 2);

					// seek to just after the header
					rpakStream.seekg(0x58, std::ios::beg);
					// read the starpak reference(s)
					std::vector<char> buf(starpaksSize);
					rpakStream.read(buf.data(), starpaksSize);

					rpakStream.close();

					// split the starpak reference(s) into strings to hash
					std::string str = "";
					for (int i = 0; i < starpaksSize; i++)
					{
						// if the current char is null, that signals the end of the current starpak path
						if (buf[i] != 0x00)
						{
							str += buf[i];
						}
						else
						{
							// only add the string we are making if it isnt empty
							if (!str.empty())
							{
								mod.StarpakPaths.push_back(STR_HASH(str));
								spdlog::info("Mod {} registered starpak '{}'", mod.Name, str);
								str = "";
							}
						}
					}

					// not using atm because we need to resolve path to rpak
					// if (m_hasLoadedMods && modPak.m_bAutoLoad)
					//	g_pPakLoadManager->LoadPakAsync(pakName.c_str());
				}
			}
		}

		// read keyvalues paths
		if (fs::exists(mod.m_ModDirectory / "keyvalues"))
		{
			for (fs::directory_entry file : fs::recursive_directory_iterator(mod.m_ModDirectory / "keyvalues"))
			{
				if (fs::is_regular_file(file))
				{
					std::string kvStr =
						g_pModManager->NormaliseModFilePath(file.path().lexically_relative(mod.m_ModDirectory / "keyvalues"));
					mod.KeyValues.emplace(STR_HASH(kvStr), kvStr);
				}
			}
		}

		// read pdiff
		if (fs::exists(mod.m_ModDirectory / "mod.pdiff"))
		{
			std::ifstream pdiffStream(mod.m_ModDirectory / "mod.pdiff");

			if (!pdiffStream.fail())
			{
				std::stringstream pdiffStringStream;
				while (pdiffStream.peek() != EOF)
					pdiffStringStream << (char)pdiffStream.get();

				pdiffStream.close();

				mod.Pdiff = pdiffStringStream.str();
			}
		}

		// read bink video paths
		if (fs::exists(mod.m_ModDirectory / "media"))
		{
			for (fs::directory_entry file : fs::recursive_directory_iterator(mod.m_ModDirectory / "media"))
				if (fs::is_regular_file(file) && file.path().extension() == ".bik")
					mod.BinkVideos.push_back(file.path().filename().string());
		}

		// try to load audio
		if (fs::exists(mod.m_ModDirectory / "audio"))
		{
			for (fs::directory_entry file : fs::directory_iterator(mod.m_ModDirectory / "audio"))
			{
				if (fs::is_regular_file(file) && file.path().extension().string() == ".json")
				{
					if (!g_CustomAudioManager.TryLoadAudioOverride(file.path()))
					{
						spdlog::warn("Mod {} has an invalid audio def {}", mod.Name, file.path().filename().string());
						continue;
					}
				}
			}
		}
	}

	// If there are new mods, we write entries accordingly in enabledmods.json
	if (newModsDetected)
	{
		std::string outPath = GetNorthstarPrefix() + "/enabledmods.json";
		yyjson_mut_write_file(outPath.c_str(), m_EnabledModsCfg, 0, NULL, NULL);
	}

	// in a seperate loop because we register mod files in reverse order, since mods loaded later should have their files prioritised
	for (int64_t i = m_LoadedMods.size() - 1; i > -1; i--)
	{
		if (!m_LoadedMods[i].m_bEnabled)
			continue;

		if (fs::exists(m_LoadedMods[i].m_ModDirectory / MOD_OVERRIDE_DIR))
		{
			for (fs::directory_entry file : fs::recursive_directory_iterator(m_LoadedMods[i].m_ModDirectory / MOD_OVERRIDE_DIR))
			{
				std::string path =
					g_pModManager->NormaliseModFilePath(file.path().lexically_relative(m_LoadedMods[i].m_ModDirectory / MOD_OVERRIDE_DIR));
				if (file.is_regular_file() && m_ModFiles.find(path) == m_ModFiles.end())
				{
					ModOverrideFile modFile;
					modFile.m_pOwningMod = &m_LoadedMods[i];
					modFile.m_Path = path;
					m_ModFiles.insert(std::make_pair(path, modFile));
				}
			}
		}
	}

	// build modinfo obj for masterserver
	yyjson_mut_doc *doc = yyjson_mut_doc_new(&YYJSON_ALLOCATOR);
	yyjson_mut_val *root = yyjson_mut_obj(doc);
	yyjson_mut_doc_set_root(doc, root);
	yyjson_mut_val* mods = yyjson_mut_obj_add_arr(doc, root, "Mods");

	int currentModIndex = 0;
	for (Mod& mod : m_LoadedMods)
	{
		if (!mod.m_bEnabled || (!mod.RequiredOnClient && !mod.Pdiff.size()))
			continue;

		yyjson_mut_val* modObj = yyjson_mut_arr_add_obj(doc, mods);
		yyjson_mut_obj_add_strcpy(doc, modObj, "Name", mod.Name.c_str());
		yyjson_mut_obj_add_strcpy(doc, modObj, "Version", mod.Version.c_str());
		yyjson_mut_obj_add_bool(doc, modObj, "RequiredOnClient", mod.RequiredOnClient);
		yyjson_mut_obj_add_strcpy(doc, modObj, "Pdiff", mod.Pdiff.c_str());

		currentModIndex++;
	}


	const char *json = yyjson_mut_write(doc, 0, NULL);
	g_pMasterServerManager->m_sOwnModInfoJson = std::string(json);

	_free_base((void*)json);
	yyjson_mut_doc_free(doc);

	m_bHasLoadedMods = true;
}

void ModManager::UnloadMods()
{
	// clean up stuff from mods before we unload
	m_ModFiles.clear();
	fs::remove_all(GetCompiledAssetsPath());

	g_CustomAudioManager.ClearAudioOverrides();

	if (!m_bHasEnabledModsCfg)
	{
		yyjson_mut_val* root = yyjson_mut_obj(m_EnabledModsCfg);
		yyjson_mut_doc_set_root(m_EnabledModsCfg, root);
	}

	for (Mod& mod : m_LoadedMods)
	{
		// remove all built kvs
		for (std::pair<size_t, std::string> kvPaths : mod.KeyValues)
			fs::remove(GetCompiledAssetsPath() / fs::path(kvPaths.second).lexically_relative(mod.m_ModDirectory));

		mod.KeyValues.clear();

		// write to m_enabledModsCfg
		// should we be doing this here or should scripts be doing this manually?
		// main issue with doing this here is when we reload mods for connecting to a server, we write enabled mods, which isn't necessarily
		// what we wanna do

		yyjson_mut_val* root = yyjson_mut_doc_get_root(m_EnabledModsCfg);
		yyjson_mut_val* hasMod = yyjson_mut_obj_get(root, mod.Name.c_str());
		if (!hasMod)
			yyjson_mut_obj_add_bool(m_EnabledModsCfg, root, mod.Name.c_str(), mod.m_bEnabled);
		else
			yyjson_mut_set_bool(hasMod, mod.m_bEnabled);
	}

	std::string outPath = GetNorthstarPrefix() + "/enabledmods.json";
	yyjson_mut_write_file(outPath.c_str(), m_EnabledModsCfg, 0, NULL, NULL);

	// do we need to dealloc individual entries in m_loadedMods? idk, rework
	m_LoadedMods.clear();
}

std::string ModManager::NormaliseModFilePath(const fs::path path)
{
	std::string str = path.lexically_normal().string();

	// force to lowercase
	for (char& c : str)
		if (c <= 'Z' && c >= 'A')
			c = c - ('Z' - 'z');

	return str;
}

void ModManager::CompileAssetsForFile(const char* filename)
{
	size_t fileHash = STR_HASH(NormaliseModFilePath(fs::path(filename)));

	if (fileHash == m_hScriptsRsonHash)
		BuildScriptsRson();
	else if (fileHash == m_hPdefHash)
		BuildPdef();
	else if (fileHash == m_hKBActHash)
		BuildKBActionsList();
	else
	{
		// check if we should build keyvalues, depending on whether any of our mods have patch kvs for this file
		for (Mod& mod : m_LoadedMods)
		{
			if (!mod.m_bEnabled)
				continue;

			if (mod.KeyValues.find(fileHash) != mod.KeyValues.end())
			{
				TryBuildKeyValues(filename);
				return;
			}
		}
	}
}

void ConCommand_reload_mods(const CCommand& args)
{
	NOTE_UNUSED(args);
	g_pModManager->LoadMods();
}

fs::path GetModFolderPath()
{
	return fs::path(GetNorthstarPrefix() + MOD_FOLDER_SUFFIX);
}
fs::path GetThunderstoreModFolderPath()
{
	return fs::path(GetNorthstarPrefix() + THUNDERSTORE_MOD_FOLDER_SUFFIX);
}
fs::path GetRemoteModFolderPath()
{
	return fs::path(GetNorthstarPrefix() + REMOTE_MOD_FOLDER_SUFFIX);
}
fs::path GetCompiledAssetsPath()
{
	return fs::path(GetNorthstarPrefix() + COMPILED_ASSETS_SUFFIX);
}

ON_DLL_LOAD_RELIESON("engine.dll", ModManager, (ConCommand, MasterServer), (CModule module))
{
	g_pModManager = new ModManager;

	RegisterConCommand("reload_mods", ConCommand_reload_mods, "reloads mods", FCVAR_NONE);
}
