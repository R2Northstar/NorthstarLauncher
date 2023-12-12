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
#include "util/wininfo.h"
#include "core/sourceinterface.h"
#include "pluginbackend.h"
#include "logging/logging.h"
#include "dedicated/dedicated.h"

PluginManager* g_pPluginManager;

bool isValidSquirrelIdentifier(std::string s)
{
	if (!s.size())
		return false; // identifiers can't be empty
	if (s[0] <= 57)
		return false; // identifier can't start with a number
	for (char& c : s)
	{
		// only allow underscores, 0-9, A-Z and a-z
		if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_')
			continue;
		return false;
	}
	return true;
}

Plugin::Plugin(std::string path)
	: handle(g_pPluginManager->GetNewHandle()), location(path),
	  initData({.northstarModule = g_NorthstarModule, .pluginHandle = this->handle})
{
	NS::log::PLUGINSYS->info("Loading plugin at '{}'", path);

	this->module = LoadLibraryExA(path.c_str(), 0, LOAD_LIBRARY_SEARCH_USER_DIRS | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);

	if (!this->module)
	{
		NS::log::PLUGINSYS->error("Failed to load main plugin library '{}' (Error: {})", path, GetLastError());
		return;
	}

	NS::log::PLUGINSYS->info("loading interface getter");
	CreateInterfaceFn CreatePluginInterface = (CreateInterfaceFn)GetProcAddress(this->module, "CreateInterface");

	if (!CreatePluginInterface)
	{
		NS::log::PLUGINSYS->error("Plugin at '{}' does not expose CreateInterface()", path);
		return;
	}

	NS::log::PLUGINSYS->info("loading plugin id");
	this->pluginId = (IPluginId*)CreatePluginInterface("PluginId001", 0);

	if (!this->pluginId)
	{
		NS::log::PLUGINSYS->error("Could not load IPluginId interface of plugin at '{}'", path);
		return;
	}

	NS::log::PLUGINSYS->info("loading properties");
	char* name = (char*)this->GetProperty(PluginPropertyKey::NAME);
	char* logName = (char*)this->GetProperty(PluginPropertyKey::LOG_NAME);
	char* dependencyName = (char*)this->GetProperty(PluginPropertyKey::DEPENDENCY_NAME);
	int64_t context =
		(int64_t)this->pluginId->GetProperty(PluginPropertyKey::CONTEXT); // this shit crashes when I made it a union idk skill issue
	this->runOnServer = context & PluginContext::DEDICATED;
	this->runOnClient = context & PluginContext::CLIENT;

	/*
	//int64_t test = this->GetProperty(PluginPropertyKey::NAME);
	char* test = (char*)this->GetProperty(PluginPropertyKey::NAME);
	NS::log::PLUGINSYS->info("PLUGIN NAME IS {}", test);

	const char* name = "TEST";
	const char* logName = "TEST";
	const char* dependencyName = "TEST";
	*/

	this->name = std::string(name);
	this->logName = std::string(logName);
	this->dependencyName = std::string(dependencyName);
	//	this->runOnServer = context & PluginContext::DEDICATED;
	//	this->runOnClient = context & PluginContext::CLIENT;

	if (!name)
	{
		NS::log::PLUGINSYS->error("Could not load name of plugin at '{}'", path);
		return;
	}

	if (!logName)
	{
		NS::log::PLUGINSYS->error("Could not load logName of plugin {}", name);
		return;
	}

	if (!dependencyName)
	{
		NS::log::PLUGINSYS->error("Could not load dependencyName of plugin {}", name);
		return;
	}

	if (!isValidSquirrelIdentifier(this->dependencyName))
	{
		NS::log::PLUGINSYS->error("Dependency name \"{}\" of plugin {} is not valid", dependencyName, name);
		return;
	}

	NS::log::PLUGINSYS->info("loading callbacks");
	this->callbacks = (IPluginCallbacks*)CreatePluginInterface("PluginCallbacks001", 0);

	if (!this->callbacks)
	{
		NS::log::PLUGINSYS->error("Could not create callback interface of plugin {}", name);
		return;
	}

	this->logger = std::make_shared<ColoredLogger>(this->logName, NS::Colors::PLUGIN);
	RegisterLogger(this->logger);

	if (IsDedicatedServer() && !this->runOnServer)
	{
		NS::log::PLUGINSYS->error("Plugin {} did not request to run on dedicated servers", this->name);
		return;
	}

	if (!IsDedicatedServer() && !this->runOnClient)
	{
		NS::log::PLUGINSYS->warn("Plugin {} did not request to run on clients", this->name);
		return;
	}

	this->valid = true;
}

void Plugin::Unload()
{
	if (!this->module)
		return;

	if (!FreeLibrary(this->module))
	{
		NS::log::PLUGINSYS->error("Failed to unload plugin at '{}'", this->location);
		return;
	}

	this->module = 0;
	this->valid = false;
}

// TODO throw this shit out
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

std::optional<Plugin*> PluginManager::GetPlugin(int handle)
{
	if (handle < 0 || handle >= this->m_vLoadedPlugins.size())
		return std::nullopt;
	return &this->m_vLoadedPlugins[handle];
}

std::optional<Plugin> PluginManager::LoadPlugin(fs::path path)
{
	Plugin plugin = Plugin(path.string());

	if (!plugin.IsValid())
	{
		NS::log::PLUGINSYS->warn("Unloading plugin '{}' because it's invalid", path.string());
		plugin.Unload();
		return std::nullopt;
	}

	m_vLoadedPlugins.push_back(plugin);
	plugin.Init();

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
		LoadPlugin(path);
	}

	for (Plugin& plugin : m_vLoadedPlugins)
	{
		plugin.Finalize();
	}

	return true;
}

int PluginManager::GetNewHandle()
{
	return this->m_vLoadedPlugins.size();
}

void PluginManager::InformSQVMLoad(ScriptContext context, SquirrelFunctions* s)
{
	/*
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
	*/
}

void PluginManager::InformSQVMCreated(ScriptContext context, CSquirrelVM* sqvm)
{
	/*
	for (auto plugin : m_vLoadedPlugins)
	{
		if (plugin.inform_sqvm_created != NULL)
		{
			plugin.inform_sqvm_created(context, sqvm);
		}
	}
	*/
}

void PluginManager::InformSQVMDestroyed(ScriptContext context)
{
	/*
	for (auto plugin : m_vLoadedPlugins)
	{
		if (plugin.inform_sqvm_destroyed != NULL)
		{
			plugin.inform_sqvm_destroyed(context);
		}
	}
	*/
}

void PluginManager::InformDLLLoad(const char* dll, void* data, void* dllPtr)
{
	/*
	for (auto plugin : m_vLoadedPlugins)
	{
		if (plugin.inform_dll_load != NULL)
		{
			plugin.inform_dll_load(dll, (PluginEngineData*)data, dllPtr);
		}
	}
	*/
}

void PluginManager::RunFrame()
{
	/*
	for (auto plugin : m_vLoadedPlugins)
	{
		if (plugin.run_frame != NULL)
		{
			plugin.run_frame();
		}
	}
	*/
}
