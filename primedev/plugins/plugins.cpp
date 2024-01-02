#include "plugins.h"
<<<<<<< HEAD
=======
#include "config/profile.h"
>>>>>>> main

#include "squirrel/squirrel.h"
#include "plugins.h"
#include "masterserver/masterserver.h"
#include "core/convar/convar.h"
#include "server/serverpresence.h"
#include <optional>
<<<<<<< HEAD
#include <stdint.h>

#include "util/wininfo.h"
#include "core/sourceinterface.h"
#include "logging/logging.h"
#include "dedicated/dedicated.h"

#include "pluginmanager.h"

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

Plugin::Plugin(std::string path) : location(path)
{
	HMODULE m = GetModuleHandleA(path.c_str());

	if (m)
	{
		// plugins may refuse to get unloaded for any reason so we need to prevent them getting loaded twice when reloading plugins
		NS::log::PLUGINSYS->warn("Plugin has already been loaded");
		return;
	}

	this->handle = LoadLibraryExA(path.c_str(), 0, LOAD_LIBRARY_SEARCH_USER_DIRS | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);

	NS::log::PLUGINSYS->info("module handle at path {}", (void*)m);

	NS::log::PLUGINSYS->info("loaded plugin handle {}", static_cast<void*>(this->handle));

	if (!this->handle)
	{
		NS::log::PLUGINSYS->error("Failed to load main plugin library '{}' (Error: {})", path, GetLastError());
		return;
	}

	this->initData = {.pluginHandle = this->handle};

	CreateInterfaceFn CreatePluginInterface = (CreateInterfaceFn)GetProcAddress(this->handle, "CreateInterface");

	if (!CreatePluginInterface)
	{
		NS::log::PLUGINSYS->error("Plugin at '{}' does not expose CreateInterface()", path);
		return;
	}

	this->pluginId = (IPluginId*)CreatePluginInterface(PLUGIN_ID_VERSION, 0);

	if (!this->pluginId)
	{
		NS::log::PLUGINSYS->error("Could not load IPluginId interface of plugin at '{}'", path);
		return;
	}

	const char* name = this->pluginId->GetString(PluginString::NAME);
	const char* logName = this->pluginId->GetString(PluginString::LOG_NAME);
	const char* dependencyName = this->pluginId->GetString(PluginString::DEPENDENCY_NAME);
	int64_t context = this->pluginId->GetField(PluginField::CONTEXT);

	this->runOnServer = context & PluginContext::DEDICATED;
	this->runOnClient = context & PluginContext::CLIENT;

	this->name = std::string(name);
	this->logName = std::string(logName);
	this->dependencyName = std::string(dependencyName);

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

bool Plugin::Unload()
{
	if (!this->handle)
		return true;

	if (this->IsValid())
	{
		bool unloaded = this->callbacks->Unload();

		if (!unloaded)
			return false;
	}

	if (!FreeLibrary(this->handle))
	{
		NS::log::PLUGINSYS->error("Failed to unload plugin at '{}'", this->location);
		return true;
	}

	g_pPluginManager->RemovePlugin(this->handle);
	return true;
}

void Plugin::Reload()
{
	std::string path = this->location;
	bool unloaded = this->Unload();

	if (!unloaded)
		return;

	g_pPluginManager->LoadPlugin(fs::path(path), true);
}

void Plugin::Log(spdlog::level::level_enum level, char* msg)
{
	this->logger->log(level, msg);
}

bool Plugin::IsValid() const
{
	return this->valid;
}

std::string Plugin::GetName() const
{
	return this->name;
}

std::string Plugin::GetLogName() const
{
	return this->logName;
}

std::string Plugin::GetDependencyName() const
{
	return this->dependencyName;
}

bool Plugin::ShouldRunOnServer() const
{
	return this->runOnServer;
}

bool Plugin::ShouldRunOnClient() const
{
	return this->runOnClient;
}

void* Plugin::CreateInterface(const char* name, int* status) const
{
	return this->m_pCreateInterface(name, status);
}

void Plugin::Init(bool reloaded)
{
	this->callbacks->Init(g_NorthstarModule, &this->initData, reloaded);
}

void Plugin::Finalize()
{
	this->callbacks->Finalize();
}

void Plugin::OnSqvmCreated(CSquirrelVM* sqvm)
{
	this->callbacks->OnSqvmCreated(sqvm);
}

void Plugin::OnSqvmDestroying(CSquirrelVM* sqvm)
{
	NS::log::PLUGINSYS->info("destroying sqvm {}", sqvm->vmContext);
	this->callbacks->OnSqvmDestroying(sqvm);
}

void Plugin::OnLibraryLoaded(HMODULE module, const char* name)
{
	this->callbacks->OnLibraryLoaded(module, name);
}

void Plugin::RunFrame()
{
	this->callbacks->RunFrame();
=======
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
	std::wstring wpath = path.wstring();

	LPCWSTR wpptr = wpath.c_str();
	HMODULE datafile = LoadLibraryExW(wpptr, 0, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE); // Load the DLL as a data file
	if (datafile == NULL)
	{
		NS::log::PLUGINSYS->info("Failed to load library '{}': ", std::system_category().message(GetLastError()));
		return std::nullopt;
	}
	HRSRC manifestResource = FindResourceW(datafile, MAKEINTRESOURCEW(IDR_RCDATA1), RT_RCDATA);

	if (manifestResource == NULL)
	{
		NS::log::PLUGINSYS->info("Could not find manifest for library '{}'", pathstring);
		freeLibrary(datafile);
		return std::nullopt;
	}
	HGLOBAL myResourceData = LoadResource(datafile, manifestResource);
	if (myResourceData == NULL)
	{
		NS::log::PLUGINSYS->error("Failed to load manifest from library '{}'", pathstring);
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
		NS::log::PLUGINSYS->error("Manifest for '{}' was invalid", pathstring);
		return std::nullopt;
	}
	if (!manifestJSON.HasMember("name"))
	{
		NS::log::PLUGINSYS->error("'{}' is missing a name in its manifest", pathstring);
		return std::nullopt;
	}
	if (!manifestJSON.HasMember("displayname"))
	{
		NS::log::PLUGINSYS->error("'{}' is missing a displayname in its manifest", pathstring);
		return std::nullopt;
	}
	if (!manifestJSON.HasMember("description"))
	{
		NS::log::PLUGINSYS->error("'{}' is missing a description in its manifest", pathstring);
		return std::nullopt;
	}
	if (!manifestJSON.HasMember("api_version"))
	{
		NS::log::PLUGINSYS->error("'{}' is missing a api_version in its manifest", pathstring);
		return std::nullopt;
	}
	if (!manifestJSON.HasMember("version"))
	{
		NS::log::PLUGINSYS->error("'{}' is missing a version in its manifest", pathstring);
		return std::nullopt;
	}
	if (!manifestJSON.HasMember("run_on_server"))
	{
		NS::log::PLUGINSYS->error("'{}' is missing 'run_on_server' in its manifest", pathstring);
		return std::nullopt;
	}
	if (!manifestJSON.HasMember("run_on_client"))
	{
		NS::log::PLUGINSYS->error("'{}' is missing 'run_on_client' in its manifest", pathstring);
		return std::nullopt;
	}
	auto test = manifestJSON["api_version"].GetString();
	if (strcmp(manifestJSON["api_version"].GetString(), std::to_string(ABI_VERSION).c_str()))
	{
		NS::log::PLUGINSYS->error(
			"'{}' has an incompatible API version number in its manifest. Current ABI version is '{}'", pathstring, ABI_VERSION);
		return std::nullopt;
	}
	// Passed all checks, going to actually load it now

	HMODULE pluginLib =
		LoadLibraryExW(wpptr, 0, LOAD_LIBRARY_SEARCH_USER_DIRS | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS); // Load the DLL with lib folders
	if (pluginLib == NULL)
	{
		NS::log::PLUGINSYS->info("Failed to load library '{}': ", std::system_category().message(GetLastError()));
		return std::nullopt;
	}
	plugin.init = (PLUGIN_INIT_TYPE)GetProcAddress(pluginLib, "PLUGIN_INIT");
	if (plugin.init == NULL)
	{
		NS::log::PLUGINSYS->info("Library '{}' has no function 'PLUGIN_INIT'", pathstring);
		return std::nullopt;
	}
	NS::log::PLUGINSYS->info("Succesfully loaded {}", pathstring);

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

	if (std::find_if(
			plugin.dependencyName.begin(),
			plugin.dependencyName.end(),
			[&](char c) -> bool { return !((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_'); }) !=
		plugin.dependencyName.end())
	{
		NS::log::PLUGINSYS->warn("Dependency string \"{}\" in {} is not valid a squirrel constant!", plugin.dependencyName, plugin.name);
	}

	plugin.init_sqvm_client = (PLUGIN_INIT_SQVM_TYPE)GetProcAddress(pluginLib, "PLUGIN_INIT_SQVM_CLIENT");
	plugin.init_sqvm_server = (PLUGIN_INIT_SQVM_TYPE)GetProcAddress(pluginLib, "PLUGIN_INIT_SQVM_SERVER");
	plugin.inform_sqvm_created = (PLUGIN_INFORM_SQVM_CREATED_TYPE)GetProcAddress(pluginLib, "PLUGIN_INFORM_SQVM_CREATED");
	plugin.inform_sqvm_destroyed = (PLUGIN_INFORM_SQVM_DESTROYED_TYPE)GetProcAddress(pluginLib, "PLUGIN_INFORM_SQVM_DESTROYED");

	plugin.inform_dll_load = (PLUGIN_INFORM_DLL_LOAD_TYPE)GetProcAddress(pluginLib, "PLUGIN_INFORM_DLL_LOAD");

	plugin.run_frame = (PLUGIN_RUNFRAME)GetProcAddress(pluginLib, "PLUGIN_RUNFRAME");

	plugin.handle = m_vLoadedPlugins.size();
	plugin.logger = std::make_shared<ColoredLogger>(plugin.displayName.c_str(), NS::Colors::PLUGIN);
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
>>>>>>> main
}
