#include "plugins.h"

#include "squirrel/squirrel.h"
#include "plugins.h"
#include "masterserver/masterserver.h"
#include "core/convar/convar.h"
#include "server/serverpresence.h"
#include <optional>
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

Plugin::Plugin(std::string path)
	: handle(g_pPluginManager->GetNewHandle()), location(path),
	  initData({.pluginHandle = this->handle})
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
	this->pluginId = (IPluginId*)CreatePluginInterface(PLUGIN_ID_VERSION, 0);

	if (!this->pluginId)
	{
		NS::log::PLUGINSYS->error("Could not load IPluginId interface of plugin at '{}'", path);
		return;
	}

	NS::log::PLUGINSYS->info("loading properties");
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

	this->callbacks->Unload();

	if (!FreeLibrary(this->module))
	{
		NS::log::PLUGINSYS->error("Failed to unload plugin at '{}'", this->location);
		return;
	}

	this->module = 0;
	this->valid = false;
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
};

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

void Plugin::Init()
{
	this->callbacks->Init(g_NorthstarModule, &this->initData);
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
	this->callbacks->OnSqvmDestroying(sqvm);
}

void Plugin::OnLibraryLoaded(HMODULE module, const char* name)
{
	this->callbacks->OnLibraryLoaded(module, name);
}

void Plugin::RunFrame()
{
	this->callbacks->RunFrame();
}

