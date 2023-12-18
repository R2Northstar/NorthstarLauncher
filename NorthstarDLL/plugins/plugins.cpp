#include "plugins.h"
#include "config/profile.h"

#include "squirrel/squirrel.h"
#include "plugins.h"
#include "masterserver/masterserver.h"
#include "core/convar/convar.h"
#include "server/serverpresence.h"
#include <optional>
#include <regex>

#include "util/version.h"
#include "pluginbackend.h"
#include "util/wininfo.h"
#include "logging/logging.h"
#include "dedicated/dedicated.h"

PluginManager* g_pPluginManager;

void freeLibrary(HMODULE hLib)
{
	if (!FreeLibrary(hLib))
	{
		Error(eLog::PLUGSYS, NO_ERROR, "There was an error while trying to free library\n");
	}
}

EXPORT void PLUGIN_LOG(LogMsg* msg)
{
	eLogLevel eLevel = eLogLevel::LOG_INFO;

	switch (msg->level)
	{
	case spdlog::level::level_enum::debug:
	case spdlog::level::level_enum::info:
		eLevel = eLogLevel::LOG_INFO;
		break;
	case spdlog::level::level_enum::off:
	case spdlog::level::level_enum::trace:
	case spdlog::level::level_enum::warn:
		eLevel = eLogLevel::LOG_WARN;
		break;
	case spdlog::level::level_enum::critical:
	case spdlog::level::level_enum::err:
		eLevel = eLogLevel::LOG_ERROR;
		break;
	}

	int hPlugin = msg->pluginHandle;
	if (hPlugin < g_pPluginManager->m_vLoadedPlugins.size() && hPlugin >= 0)
	{
		const char* pszName = g_pPluginManager->m_vLoadedPlugins[hPlugin].displayName.c_str();
		PluginMsg(eLevel, pszName, "%s\n", msg->msg);
	}
	else
	{
		DevMsg(eLog::PLUGSYS, "Invalid plugin handle: %i\n", hPlugin);
	}
}

EXPORT void* CreateObject(ObjectType type)
{
	switch (type)
	{
	case ObjectType::CONVAR:
		return (void*)new ConVar;
	case ObjectType::CONCOMMANDS:
		return (void*)new ConCommand;
	default:
		return NULL;
	}
}

std::optional<Plugin> PluginManager::LoadPlugin(fs::path path, PluginInitFuncs* funcs, PluginNorthstarData* data)
{

	Plugin plugin {};

	std::string pathstring = path.string();
	std::wstring wpath = path.wstring();

	LPCWSTR wpptr = wpath.c_str();
	HMODULE datafile = LoadLibraryExW(wpptr, 0, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE); // Load the DLL as a data file
	if (datafile == NULL)
	{
		Error(
			eLog::PLUGSYS,
			NO_ERROR,
			"Failed to load library '%s': %s\n",
			pathstring.c_str(),
			std::system_category().message(GetLastError()).c_str());
		return std::nullopt;
	}
	HRSRC manifestResource = FindResourceW(datafile, MAKEINTRESOURCEW(IDR_RCDATA1), RT_RCDATA);

	if (manifestResource == NULL)
	{
		Error(eLog::PLUGSYS, NO_ERROR, "Could not find manifest for library '%s'\n", pathstring.c_str());
		freeLibrary(datafile);
		return std::nullopt;
	}
	HGLOBAL myResourceData = LoadResource(datafile, manifestResource);
	if (myResourceData == NULL)
	{
		Error(eLog::PLUGSYS, NO_ERROR, "Failed to load manifest from library '%s'\n", pathstring.c_str());
		freeLibrary(datafile);
		return std::nullopt;
	}
	int manifestSize = SizeofResource(datafile, manifestResource);
	std::string manifest = std::string((const char*)LockResource(myResourceData), 0, manifestSize);
	freeLibrary(datafile);

	rapidjson_document manifestJSON;
	manifestJSON.Parse(manifest.c_str());

	if (manifestJSON.HasParseError())
	{
		Error(eLog::PLUGSYS, NO_ERROR, "Manifest for '%s' was invalid\n", pathstring.c_str());
		return std::nullopt;
	}
	if (!manifestJSON.HasMember("name"))
	{
		Error(eLog::PLUGSYS, NO_ERROR, "'%s' is missing a name in its manifest\n", pathstring.c_str());
		return std::nullopt;
	}
	if (!manifestJSON.HasMember("displayname"))
	{
		Error(eLog::PLUGSYS, NO_ERROR, "'%s' is missing a displayname in its manifest\n", pathstring.c_str());
		return std::nullopt;
	}
	if (!manifestJSON.HasMember("description"))
	{
		Error(eLog::PLUGSYS, NO_ERROR, "'%s' is missing a description in its manifest\n", pathstring.c_str());
		return std::nullopt;
	}
	if (!manifestJSON.HasMember("api_version"))
	{
		Error(eLog::PLUGSYS, NO_ERROR, "'%s' is missing a api_version in its manifest\n", pathstring.c_str());
		return std::nullopt;
	}
	if (!manifestJSON.HasMember("version"))
	{
		Error(eLog::PLUGSYS, NO_ERROR, "'%s' is missing a version in its manifest\n", pathstring.c_str());
		return std::nullopt;
	}
	if (!manifestJSON.HasMember("run_on_server"))
	{
		Error(eLog::PLUGSYS, NO_ERROR, "'%s' is missing 'run_on_server' in its manifest\n", pathstring.c_str());
		return std::nullopt;
	}
	if (!manifestJSON.HasMember("run_on_client"))
	{
		Error(eLog::PLUGSYS, NO_ERROR, "'%s' is missing 'run_on_client' in its manifest\n", pathstring.c_str());
		return std::nullopt;
	}
	auto test = manifestJSON["api_version"].GetString();
	if (strcmp(manifestJSON["api_version"].GetString(), std::to_string(ABI_VERSION).c_str()))
	{
		Error(
			eLog::PLUGSYS,
			NO_ERROR,
			"'%s' has an incompatible API version number '%s' in its manifest. Current ABI version is '%i'\n",
			pathstring.c_str(),
			manifestJSON["api_version"].GetString(),
			ABI_VERSION);
		return std::nullopt;
	}
	// Passed all checks, going to actually load it now

	HMODULE pluginLib = LoadLibraryW(wpptr); // Load the DLL as a data file
	if (pluginLib == NULL)
	{
		Error(
			eLog::PLUGSYS,
			NO_ERROR,
			"Failed to load library '%s': %s\n",
			pathstring.c_str(),
			std::system_category().message(GetLastError()).c_str());
		return std::nullopt;
	}
	plugin.init = (PLUGIN_INIT_TYPE)GetProcAddress(pluginLib, "PLUGIN_INIT");
	if (plugin.init == NULL)
	{
		Error(eLog::PLUGSYS, NO_ERROR, "Library '%s' has no function 'PLUGIN_INIT'\n", pathstring.c_str());
		return std::nullopt;
	}
	DevMsg(eLog::PLUGSYS, "Succesfully loaded %s\n", pathstring.c_str());

	plugin.name = manifestJSON["name"].GetString();
	plugin.displayName = manifestJSON["displayname"].GetString();
	plugin.description = manifestJSON["description"].GetString();
	plugin.api_version = manifestJSON["api_version"].GetString();
	plugin.version = manifestJSON["version"].GetString();

	plugin.run_on_client = manifestJSON["run_on_client"].GetBool();
	plugin.run_on_server = manifestJSON["run_on_server"].GetBool();

	if (!plugin.run_on_server && IsDedicatedServer())
		return std::nullopt;

	if (manifestJSON.HasMember("dependencyName"))
	{
		plugin.dependencyName = manifestJSON["dependencyName"].GetString();
	}
	else
	{
		plugin.dependencyName = plugin.name;
	}

	plugin.init_sqvm_client = (PLUGIN_INIT_SQVM_TYPE)GetProcAddress(pluginLib, "PLUGIN_INIT_SQVM_CLIENT");
	plugin.init_sqvm_server = (PLUGIN_INIT_SQVM_TYPE)GetProcAddress(pluginLib, "PLUGIN_INIT_SQVM_SERVER");
	plugin.inform_sqvm_created = (PLUGIN_INFORM_SQVM_CREATED_TYPE)GetProcAddress(pluginLib, "PLUGIN_INFORM_SQVM_CREATED");
	plugin.inform_sqvm_destroyed = (PLUGIN_INFORM_SQVM_DESTROYED_TYPE)GetProcAddress(pluginLib, "PLUGIN_INFORM_SQVM_DESTROYED");

	plugin.inform_dll_load = (PLUGIN_INFORM_DLL_LOAD_TYPE)GetProcAddress(pluginLib, "PLUGIN_INFORM_DLL_LOAD");

	plugin.run_frame = (PLUGIN_RUNFRAME)GetProcAddress(pluginLib, "PLUGIN_RUNFRAME");

	plugin.handle = m_vLoadedPlugins.size();
	DevMsg(eLog::PLUGSYS, "Loading plugin %s version %s\n", plugin.displayName.c_str(), plugin.version.c_str());
	m_vLoadedPlugins.push_back(plugin);

	plugin.init(funcs, data);

	return plugin;
}

inline void FindPlugins(fs::path pluginPath, std::vector<fs::path>& paths)
{
	// ensure dirs exist
	if (!fs::exists(pluginPath) || !fs::is_directory(pluginPath))
	{
		return;
	}

	for (const fs::directory_entry& entry : fs::recursive_directory_iterator(pluginPath))
	{
		if (fs::is_regular_file(entry) && entry.path().extension() == ".dll")
			paths.emplace_back(entry.path());
	}
}

bool PluginManager::LoadPlugins()
{
	if (strstr(GetCommandLineA(), "-noplugins") != NULL)
	{
		Warning(eLog::PLUGSYS, "-noplugins detected; skipping loading plugins\n");
		return false;
	}

	fs::create_directories(GetThunderstoreModFolderPath());

	std::vector<fs::path> paths;

	pluginPath = GetNorthstarPrefix() + "\\plugins";

	PluginNorthstarData data {};
	std::string ns_version {version};

	PluginInitFuncs funcs {};
	funcs.logger = PLUGIN_LOG;
	funcs.relayInviteFunc = nullptr;
	funcs.createObject = CreateObject;

	data.version = ns_version.c_str();
	data.northstarModule = g_NorthstarModule;

	FindPlugins(pluginPath, paths);

	// Special case for Thunderstore mods dir
	std::filesystem::directory_iterator thunderstoreModsDir = fs::directory_iterator(GetThunderstoreModFolderPath());
	// Set up regex for `AUTHOR-MOD-VERSION` pattern
	std::regex pattern(R"(.*\\([a-zA-Z0-9_]+)-([a-zA-Z0-9_]+)-(\d+\.\d+\.\d+))");
	for (fs::directory_entry dir : thunderstoreModsDir)
	{
		fs::path pluginsDir = dir.path() / "plugins";
		// Use regex to match `AUTHOR-MOD-VERSION` pattern
		if (!std::regex_match(dir.path().string(), pattern))
		{
			Warning(eLog::PLUGSYS, "The following directory did not match 'AUTHOR-MOD-VERSION': %s\n", dir.path().c_str());
			continue; // skip loading package that doesn't match
		}
		FindPlugins(pluginsDir, paths);
	}

	if (paths.empty())
	{
		Warning(eLog::PLUGSYS, "Could not find any plugins. Skipped loading plugins\n");
		return false;
	}

	for (fs::path path : paths)
	{
		if (LoadPlugin(path, &funcs, &data))
			data.pluginHandle += 1;
	}
	return true;
}

void PluginManager::InformSQVMLoad(ScriptContext context, SquirrelFunctions* s)
{
	for (auto plugin : m_vLoadedPlugins)
	{
		if (context == ScriptContext::CLIENT && plugin.init_sqvm_client != NULL)
		{
			plugin.init_sqvm_client(s);
		}
		else if (context == ScriptContext::SERVER && plugin.init_sqvm_server != NULL)
		{
			plugin.init_sqvm_server(s);
		}
	}
}

void PluginManager::InformSQVMCreated(ScriptContext context, CSquirrelVM* sqvm)
{
	for (auto plugin : m_vLoadedPlugins)
	{
		if (plugin.inform_sqvm_created != NULL)
		{
			plugin.inform_sqvm_created(context, sqvm);
		}
	}
}

void PluginManager::InformSQVMDestroyed(ScriptContext context)
{
	for (auto plugin : m_vLoadedPlugins)
	{
		if (plugin.inform_sqvm_destroyed != NULL)
		{
			plugin.inform_sqvm_destroyed(context);
		}
	}
}

void PluginManager::InformDLLLoad(const char* dll, void* data, void* dllPtr)
{
	for (auto plugin : m_vLoadedPlugins)
	{
		if (plugin.inform_dll_load != NULL)
		{
			plugin.inform_dll_load(dll, (PluginEngineData*)data, dllPtr);
		}
	}
}

void PluginManager::RunFrame()
{
	for (auto plugin : m_vLoadedPlugins)
	{
		if (plugin.run_frame != NULL)
		{
			plugin.run_frame();
		}
	}
}
