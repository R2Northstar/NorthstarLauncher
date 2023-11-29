#include "plugins.h"
#include "config/profile.h"

#include "squirrel/squirrel.h"
#include "plugins.h"
#include "masterserver/masterserver.h"
#include "core/convar/convar.h"
#include "server/serverpresence.h"
#include <optional>
#include <regex>
#include <stdint.h>

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
		spdlog::error("There was an error while trying to free library");
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
	std::wstring wpath = path.wstring(); // is there any reason to load the libs with wide strings?

	LPCWSTR wpptr = wpath.c_str();

	HMODULE pluginLib =
		LoadLibraryExW(wpptr, 0, LOAD_LIBRARY_SEARCH_USER_DIRS | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS); // Load the DLL with lib folders

	if (!pluginLib)
	{
		NS::log::PLUGINSYS->error("'Failed to load library '{}'", std::system_category().message(GetLastError())); // GetLastError??? We know the path that failed to load
		return std::nullopt;
	}

	uint32_t* abiVersion = (uint32_t*)GetProcAddress(pluginLib, "PLUGIN_ABI_VERSION");
	char** name = (char**)GetProcAddress(pluginLib, "PLUGIN_NAME");
	char** description = (char**)GetProcAddress(pluginLib, "PLUGIN_DESCRIPTION");
	char** abbreviation = (char**)GetProcAddress(pluginLib, "PLUGIN_ABBREVIATION");
	char** version = (char**)GetProcAddress(pluginLib, "PLUGIN_VERSION");
	char** dependencyName = (char**)GetProcAddress(pluginLib, "PLUGIN_DEPENDENCY_NAME");
	uint8_t* context = (uint8_t*)GetProcAddress(pluginLib, "PLUGIN_CONTEXT");
	PLUGIN_INIT_TYPE plugin_init = (PLUGIN_INIT_TYPE)GetProcAddress(pluginLib, "PLUGIN_INIT");

	// ensure all required values are found
#define ASSERT_HAS_PROP(e, prop) if (!e) { NS::log::PLUGINSYS->error("'{}' is not exporting it's {}", pathstring, prop); freeLibrary(pluginLib); return std::nullopt;; }
		ASSERT_HAS_PROP(abiVersion, "PLUGIN_ABI_VERSION")
		ASSERT_HAS_PROP(name, "PLUGIN_NAME");
		ASSERT_HAS_PROP(description, "PLUGIN_DESCRIPTION");
		ASSERT_HAS_PROP(abbreviation, "PLUGIN_ABBREVIATION");
		ASSERT_HAS_PROP(version, "PLUGIN_VERSION");
		ASSERT_HAS_PROP(context, "PLUGIN_CONTEXT");
		ASSERT_HAS_PROP(plugin_init, "PLUGIN_INIT");
#undef ASSERT_HAS_PROP

		if (ABI_VERSION != *abiVersion)
		{
			NS::log::PLUGINSYS->error(
				"'{}' has an incompatible API version number ('{}') in its manifest. Current ABI version is '{}'", pathstring, *abiVersion, ABI_VERSION);
			freeLibrary(pluginLib);
			return std::nullopt;
		}

	// Passed all checks, going to actually load it now
	// NS::log::PLUGINSYS->info("Succesfully loaded {}", pathstring); // we log the same thing 20 lines later why is this here

	plugin.name = *name;
	plugin.displayName = *abbreviation;
	plugin.description = *description;
	plugin.api_version = *abiVersion;
	plugin.version = *version;
	plugin.init = *plugin_init;

	plugin.run_on_client = PluginContext::CLIENT & *context;
	plugin.run_on_server = PluginContext::DEDICATED & *context;

	if (!plugin.run_on_server && IsDedicatedServer())
	{
		freeLibrary(pluginLib);
		return std::nullopt;
	}

	if (dependencyName)
	{
		plugin.dependencyName = *dependencyName;
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
	plugin.logger = std::make_shared<ColoredLogger>(plugin.displayName, NS::Colors::PLUGIN);
	RegisterLogger(plugin.logger);
	NS::log::PLUGINSYS->info("Loading plugin {} version {}", plugin.displayName, plugin.version);
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

	for (const fs::directory_entry& entry : fs::directory_iterator(pluginPath))
	{
		if (fs::is_regular_file(entry) && entry.path().extension() == ".dll")
			paths.emplace_back(entry.path());
	}
}

bool PluginManager::LoadPlugins()
{
	if (strstr(GetCommandLineA(), "-noplugins") != NULL)
	{
		NS::log::PLUGINSYS->warn("-noplugins detected; skipping loading plugins");
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

	fs::path libPath = fs::absolute(pluginPath + "\\lib");
	if (fs::exists(libPath) && fs::is_directory(libPath))
		AddDllDirectory(libPath.wstring().c_str());

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
			spdlog::warn("The following directory did not match 'AUTHOR-MOD-VERSION': {}", dir.path().string());
			continue; // skip loading package that doesn't match
		}

		fs::path libDir = fs::absolute(pluginsDir / "lib");
		if (fs::exists(libDir) && fs::is_directory(libDir))
			AddDllDirectory(libDir.wstring().c_str());

		FindPlugins(pluginsDir, paths);
	}

	if (paths.empty())
	{
		NS::log::PLUGINSYS->warn("Could not find any plugins. Skipped loading plugins");
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
