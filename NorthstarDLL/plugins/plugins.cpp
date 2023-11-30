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

namespace Symbol
{
	static const char* REQUIRED_ABI_VERSION = "PLUGIN_ABI_VERSION";
	static const char* NAME = "PLUGIN_NAME";
	static const char* DESCRIPTION = "PLUGIN_DESCRIPTION";
	static const char* LOG_NAME = "PLUGIN_LOG_NAME";
	static const char* VERSION = "PLUGIN_VERSION";
	static const char* DEPENDENCY_NAME = "PLUGIN_DEPENDENCY_NAME";
	static const char* CONTEXT = "PLUGIN_CONTEXT";
	static const char* INIT = "PLUGIN_INIT";
	static const char* CLIENT_SQVM_INIT = "PLUGIN_INIT_SQVM_CLIENT";
	static const char* SERVER_SQVM_INIT = "PLUGIN_INIT_SQVM_SERVER";
	static const char* SQVM_CREATED = "PLUGIN_INFORM_SQVM_CREATED";
	static const char* SQVM_DESTROYED = "PLUGIN_INFORM_SQVM_CREATED";
	static const char* LIB_LOAD = "PLUGIN_INFORM_DLL_LOAD";
	static const char* RUN_FRAME = "PLUGIN_RUNFRAME";
}; // namespace Symbol

PluginManager* g_pPluginManager;

template <typename T> T GetSymbolValue(HMODULE lib, const char* symbol)
{
	const T* addr = (T*)GetProcAddress(lib, symbol);
	return addr ? *addr : NULL;
}

Plugin::Plugin(HMODULE lib, int handle, std::string path)
	: name(GetSymbolValue<char*>(lib, Symbol::NAME)), logName(GetSymbolValue<char*>(lib, Symbol::LOG_NAME)),
	  dependencyName(GetSymbolValue<char*>(lib, Symbol::DEPENDENCY_NAME)), description(GetSymbolValue<char*>(lib, Symbol::DESCRIPTION)),
	  api_version(GetSymbolValue<uint32_t>(lib, Symbol::REQUIRED_ABI_VERSION)), version(GetSymbolValue<char*>(lib, Symbol::VERSION)),
	  handle(handle), runOnContext(GetSymbolValue<uint8_t>(lib, Symbol::CONTEXT)), run_on_client(runOnContext & PluginContext::CLIENT),
	  run_on_server(runOnContext & PluginContext::DEDICATED), init(GetSymbolValue<PLUGIN_INIT_TYPE>(lib, Symbol::INIT)),
	  init_sqvm_client(GetSymbolValue<PLUGIN_INIT_SQVM_TYPE>(lib, Symbol::CLIENT_SQVM_INIT)),
	  init_sqvm_server(GetSymbolValue<PLUGIN_INIT_SQVM_TYPE>(lib, Symbol::SERVER_SQVM_INIT)),
	  inform_sqvm_created(GetSymbolValue<PLUGIN_INFORM_SQVM_CREATED_TYPE>(lib, Symbol::SQVM_CREATED)),
	  inform_sqvm_destroyed(GetSymbolValue<PLUGIN_INFORM_SQVM_DESTROYED_TYPE>(lib, Symbol::SQVM_DESTROYED)),
	  inform_dll_load(GetSymbolValue<PLUGIN_INFORM_DLL_LOAD_TYPE>(lib, Symbol::LIB_LOAD)),
	  run_frame(GetSymbolValue<PLUGIN_RUNFRAME>(lib, Symbol::RUN_FRAME)),
	  valid(api_version && name && description && version && init && logName && runOnContext)
{
#define LOG_MISSING_SYMBOL(e, symbol)                                                                                                      \
	if (!e)                                                                                                                                \
		NS::log::PLUGINSYS->error("'{} does not export required symbol '{}'", path, symbol);
	LOG_MISSING_SYMBOL(api_version, Symbol::REQUIRED_ABI_VERSION)
	LOG_MISSING_SYMBOL(name, Symbol::NAME)
	LOG_MISSING_SYMBOL(description, Symbol::DESCRIPTION)
	LOG_MISSING_SYMBOL(version, Symbol::VERSION)
	LOG_MISSING_SYMBOL(logName, Symbol::LOG_NAME)
	LOG_MISSING_SYMBOL(runOnContext, Symbol::CONTEXT)
	LOG_MISSING_SYMBOL(init, Symbol::INIT)
#undef LOG_MISSING_SYMBOL

	if (ABI_VERSION != api_version)
	{
		NS::log::PLUGINSYS->error(
			"'{}' has an incompatible API version number ('{}') in its manifest. Current ABI version is '{}'",
			path,
			api_version,
			ABI_VERSION);
	}
}

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
	std::string pathstring = path.string();
	HMODULE pluginLib = LoadLibraryExA(
		pathstring.c_str(), 0, LOAD_LIBRARY_SEARCH_USER_DIRS | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS); // Load the DLL with lib folders

	if (!pluginLib)
	{
		NS::log::PLUGINSYS->error(
			"'Failed to load library '{}'",
			std::system_category().message(GetLastError())); // GetLastError??? We know the path that failed to load
		return std::nullopt;
	}

	Plugin plugin = Plugin(pluginLib, m_vLoadedPlugins.size(), pathstring);

	if (!plugin.valid || (!plugin.run_on_server && IsDedicatedServer())
	{
		freeLibrary(pluginLib);
		return std::nullopt;
	}

	// Passed all checks, going to actually load it now
	// NS::log::PLUGINSYS->info("Succesfully loaded {}", pathstring); // we log the same thing 20 lines later why is this here

	plugin.logger = std::make_shared<ColoredLogger>(plugin.logName, NS::Colors::PLUGIN);
	d
	RegisterLogger(plugin.logger);
	NS::log::PLUGINSYS->info("Loading plugin {} version {}", plugin.logName, plugin.version);
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
