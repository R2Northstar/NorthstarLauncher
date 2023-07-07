#include "plugins.h"
#include "config/profile.h"

#include "squirrel/squirrel.h"
#include "plugins.h"
#include "masterserver/masterserver.h"
#include "core/convar/convar.h"
#include "server/serverpresence.h"
#include <optional>

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
	spdlog::source_loc src {};
	src.filename = msg->source.file;
	src.funcname = msg->source.func;
	src.line = msg->source.line;
	auto&& logger = g_pPluginManager->m_vLoadedPlugins[msg->pluginHandle].logger;
	logger->log(src, (spdlog::level::level_enum)msg->level, msg->msg);
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
		Error(eLog::PLUGSYS, NO_ERROR, "Failed to load library '%s': %s\n", pathstring.c_str(), std::system_category().message(GetLastError()).c_str());
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

	plugin.push_presence = (PLUGIN_PUSH_PRESENCE_TYPE)GetProcAddress(pluginLib, "PLUGIN_RECEIVE_PRESENCE");

	plugin.inform_dll_load = (PLUGIN_INFORM_DLL_LOAD_TYPE)GetProcAddress(pluginLib, "PLUGIN_INFORM_DLL_LOAD");

	plugin.handle = m_vLoadedPlugins.size();
	plugin.logger = std::make_shared<ColoredLogger>(plugin.displayName.c_str(), NS::Colors::PLUGIN);
	//RegisterLogger(plugin.logger);
	DevMsg(eLog::PLUGSYS, "Loading plugin %s version %s\n", plugin.displayName.c_str(), plugin.version.c_str());
	m_vLoadedPlugins.push_back(plugin);

	plugin.init(funcs, data);

	return plugin;
}

bool PluginManager::LoadPlugins()
{
	std::vector<fs::path> paths;

	pluginPath = GetNorthstarPrefix() + "/plugins";

	PluginNorthstarData data {};
	std::string ns_version {version};

	PluginInitFuncs funcs {};
	funcs.logger = PLUGIN_LOG;
	funcs.relayInviteFunc = nullptr;
	funcs.createObject = CreateObject;

	init_plugincommunicationhandler();

	data.version = ns_version.c_str();
	data.northstarModule = g_NorthstarModule;

	if (strstr(GetCommandLineA(), "-noplugins") != NULL)
	{
		Warning(eLog::PLUGSYS, "-noplugins detected; skipping loading plugins\n");
		return false;
	}
	if (!fs::exists(pluginPath))
	{
		Warning(eLog::PLUGSYS, "Could not find a plugins directory. Skipped loading plugins\n");
		return false;
	}
	// ensure dirs exist
	fs::recursive_directory_iterator iterator(pluginPath);
	if (std::filesystem::begin(iterator) == std::filesystem::end(iterator))
	{
		Warning(eLog::PLUGSYS, "Could not find any plugins. Skipped loading plugins\n");
		return false;
	}
	for (auto const& entry : iterator)
	{
		if (fs::is_regular_file(entry) && entry.path().extension() == ".dll")
			paths.emplace_back(entry.path());
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

void PluginManager::PushPresence(PluginGameStatePresence* data)
{
	for (auto plugin : m_vLoadedPlugins)
	{
		if (plugin.push_presence != NULL)
		{
			plugin.push_presence(data);
		}
	}
}

void PluginManager::InformDLLLoad(PluginLoadDLL dll, void* data)
{
	for (auto plugin : m_vLoadedPlugins)
	{
		if (plugin.inform_dll_load != NULL)
		{
			plugin.inform_dll_load(dll, data);
		}
	}
}
