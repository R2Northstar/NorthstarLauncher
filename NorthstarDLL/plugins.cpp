#include "pch.h"
#include "plugins.h"
#include "nsprefix.h"

#include <optional>

#include "version.h"
#include "plugincommunication.h"
#include "wininfo.h"

PluginManager* g_pPluginManager;

void freeLibrary(HMODULE hLib)
{
	if (!FreeLibrary(hLib))
	{
		spdlog::error("There was an error while trying to free library");
	}
}

EXPORT void PLUGIN_LOG(LogMsg* msg)
{
	spdlog::source_loc src {};
	src.filename = msg->source.file;
	src.funcname = msg->source.func;
	src.line = msg->source.line;
	spdlog::log(src, (spdlog::level::level_enum)msg->level, msg->msg);
}

std::optional<Plugin> PluginManager::LoadPlugin(fs::path path, PluginNorthstarData* data)
{

	Plugin plugin {};

	std::string pathstring = (pluginPath / path).string();
	std::wstring wpath = (pluginPath / path).wstring();

	LPCWSTR wpptr = wpath.c_str();
	HMODULE datafile = LoadLibraryExW(wpptr, 0, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE); // Load the DLL as a data file
	if (datafile == NULL)
	{
		spdlog::info("Failed to load library {}: ", std::system_category().message(GetLastError()));
		return std::nullopt;
	}
	HRSRC manifestResource = FindResourceW(datafile, MAKEINTRESOURCEW(101), MAKEINTRESOURCEW(RT_RCDATA));

	if (manifestResource == NULL)
	{
		spdlog::info("Could not find manifest for library {}", pathstring);
		freeLibrary(datafile);
		return std::nullopt;
	}
	spdlog::info("Loading resource from library");
	HGLOBAL myResourceData = LoadResource(datafile, manifestResource);
	if (myResourceData == NULL)
	{
		spdlog::error("Failed to load resource from library");
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
		spdlog::error("Manifest for {} was invalid", pathstring);
		return std::nullopt;
	}
	if (!manifestJSON.HasMember("name"))
	{
		spdlog::error("{} is missing a name in its manifest", pathstring);
		return std::nullopt;
	}
	if (!manifestJSON.HasMember("displayname"))
	{
		spdlog::error("{} is missing a displayname in its manifest", pathstring);
		return std::nullopt;
	}
	if (!manifestJSON.HasMember("description"))
	{
		spdlog::error("{} is missing a description in its manifest", pathstring);
		return std::nullopt;
	}
	if (!manifestJSON.HasMember("api_version"))
	{
		spdlog::error("{} is missing a api_version in its manifest", pathstring);
		return std::nullopt;
	}
	if (!manifestJSON.HasMember("version"))
	{
		spdlog::error("{} is missing a version in its manifest", pathstring);
		return std::nullopt;
	}
	if (!manifestJSON.HasMember("run_on_server"))
	{
		spdlog::error("{} is missing 'run_on_server' in its manifest", pathstring);
		return std::nullopt;
	}
	if (!manifestJSON.HasMember("run_on_client"))
	{
		spdlog::error("{} is missing 'run_on_client' in its manifest", pathstring);
		return std::nullopt;
	}
	if (strcmp(manifestJSON["api_version"].GetString(), std::to_string(ABI_VERSION).c_str()))
	{
		spdlog::error("{} has an incompatible API version number in its manifest", pathstring);
		return std::nullopt;
	}
	// Passed all checks, going to actually load it now

	HMODULE pluginLib = LoadLibraryW(wpptr); // Load the DLL as a data file
	if (pluginLib == NULL)
	{
		spdlog::info("Failed to load library {}: ", std::system_category().message(GetLastError()));
		return std::nullopt;
	}
	plugin.init = (PLUGIN_INIT_TYPE)GetProcAddress(pluginLib, "PLUGIN_INIT");
	if (plugin.init == NULL)
	{
		spdlog::info("Library {} has no function initializePlugin", pathstring);
		return std::nullopt;
	}
	spdlog::info("Succesfully loaded {}", pathstring);

	plugin.name = manifestJSON["name"].GetString();
	plugin.displayName = manifestJSON["displayname"].GetString();
	plugin.description = manifestJSON["description"].GetString();
	plugin.api_version = manifestJSON["api_version"].GetString();
	plugin.version = manifestJSON["version"].GetString();

	plugin.run_on_client = manifestJSON["run_on_client"].GetBool();
	plugin.run_on_server = manifestJSON["run_on_server"].GetBool();

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

	plugin.respond_server_data = (PLUGIN_RESPOND_SERVER_DATA_TYPE)GetProcAddress(pluginLib, "PLUGIN_RESPONSE_SERVER_DATA");
	plugin.respond_gamestate_data = (PLUGIN_RESPOND_GAMESTATE_DATA_TYPE)GetProcAddress(pluginLib, "PLUGIN_RESPONSE_GAMESTATE_DATA");

	plugin.init(data);

	return plugin;
}

bool PluginManager::LoadPlugins()
{
	std::vector<fs::path> paths;

	pluginPath = GetNorthstarPrefix() + "/plugins";

	PluginNorthstarData data {};
	std::string ns_version {version};

	init_plugincommunicationhandler();

	data.version = ns_version.c_str();
	data.northstarModule = g_NorthstarModule;

	if (!fs::exists(pluginPath))
	{
		spdlog::warn("Could not find a plugins directory. Skipped loading plugins");
		return false;
	}
	// ensure dirs exist
	fs::recursive_directory_iterator iterator(pluginPath);
	if (std::filesystem::begin(iterator) == std::filesystem::end(iterator))
	{
		spdlog::warn("Could not find any plugins. Skipped loading plugins");
		return false;
	}
	for (auto const& entry : iterator)
	{
		if (fs::is_regular_file(entry) && entry.path().extension() == ".dll")
			paths.emplace_back(entry.path().filename());
	}
	for (fs::path path : paths)
	{
		auto maybe_plugin = LoadPlugin(path, &data);
		if (maybe_plugin.has_value())
		{
			m_vLoadedPlugins.push_back(maybe_plugin.value());
		}
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

void PluginManager::InformSQVMCreated(ScriptContext context, CSquirrelVM* sqvm) {
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
