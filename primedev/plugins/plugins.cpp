#include "plugins.h"
#include "pluginmanager.h"
#include "squirrel/squirrel.h"
#include "util/wininfo.h"
#include "core/sourceinterface.h"
#include "logging/logging.h"
#include "dedicated/dedicated.h"

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

Plugin::Plugin(std::string path) : m_location(path)
{
	HMODULE pluginModule = GetModuleHandleA(path.c_str());

	if (pluginModule)
	{
		// plugins may refuse to get unloaded for any reason so we need to prevent them getting loaded twice when reloading plugins
		Warning(eLog::PLUGSYS, "Plugin has already been loaded\n");
		return;
	}

	m_handle = LoadLibraryExA(path.c_str(), 0, LOAD_LIBRARY_SEARCH_USER_DIRS | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);

	DevMsg(eLog::PLUGSYS, "loaded plugin handle %p\n", static_cast<void*>(m_handle));

	if (!m_handle)
	{
		Error(eLog::PLUGSYS, NO_ERROR, "Failed to load plugin '%s' (Error: %lu)\n", path.c_str(), GetLastError());
		return;
	}

	m_initData = {.pluginHandle = m_handle};

	CreateInterfaceFn CreatePluginInterface = (CreateInterfaceFn)GetProcAddress(m_handle, "CreateInterface");

	if (!CreatePluginInterface)
	{
		Error(eLog::PLUGSYS, NO_ERROR, "Plugin at '%s' does not expose CreateInterface()\n", path.c_str());
		return;
	}

	m_pluginId = (IPluginId*)CreatePluginInterface(PLUGIN_ID_VERSION, 0);

	if (!m_pluginId)
	{
		Error(eLog::PLUGSYS, NO_ERROR, "Could not load IPluginId interface of plugin at '%s'\n", path.c_str());
		return;
	}

	const char* name = m_pluginId->GetString(PluginString::NAME);
	const char* logName = m_pluginId->GetString(PluginString::LOG_NAME);
	const char* dependencyName = m_pluginId->GetString(PluginString::DEPENDENCY_NAME);
	int64_t context = m_pluginId->GetField(PluginField::CONTEXT);

	m_runOnServer = context & PluginContext::DEDICATED;
	m_runOnClient = context & PluginContext::CLIENT;

	m_name = std::string(name);
	m_logName = std::string(logName);
	m_dependencyName = std::string(dependencyName);

	if (!name)
	{
		Error(eLog::PLUGSYS, NO_ERROR, "Could not load name of plugin at '%s'\n", path.c_str());
		return;
	}

	if (!logName)
	{
		Error(eLog::PLUGSYS, NO_ERROR, "Could not load logName of plugin %s\n", name);
		return;
	}

	if (!dependencyName)
	{
		Error(eLog::PLUGSYS, NO_ERROR, "Could not load dependencyName of plugin %s\n", name);
		return;
	}

	if (!isValidSquirrelIdentifier(m_dependencyName))
	{
		Error(eLog::PLUGSYS, NO_ERROR, "Dependency name \"%s\" of plugin %s is not valid\n", dependencyName, name);
		return;
	}

	m_callbacks = (IPluginCallbacks*)CreatePluginInterface("PluginCallbacks001", 0);

	if (!m_callbacks)
	{
		Error(eLog::PLUGSYS, NO_ERROR, "Could not create callback interface of plugin %s\n", name);
		return;
	}

	if (IsDedicatedServer() && !m_runOnServer)
	{
		DevMsg(eLog::PLUGSYS, "Unloading %s because it's not supposed to run on dedicated servers\n", m_name);
		return;
	}

	if (!IsDedicatedServer() && !m_runOnClient)
	{
		DevMsg(eLog::PLUGSYS, "Unloading %s because it's only supposed to run on dedicated servers\n", m_name);
		return;
	}

	m_valid = true;
}

bool Plugin::Unload() const
{
	if (!m_handle)
		return true;

	if (IsValid())
	{
		bool unloaded = m_callbacks->Unload();

		if (!unloaded)
			return false;
	}

	if (!FreeLibrary(m_handle))
	{
		Error(eLog::PLUGSYS, NO_ERROR, "Failed to unload plugin at '%s'\n", m_location.c_str());
		return false;
	}

	g_pPluginManager->RemovePlugin(m_handle);
	return true;
}

void Plugin::Reload() const
{
	bool unloaded = Unload();

	if (!unloaded)
		return;

	g_pPluginManager->LoadPlugin(fs::path(m_location), true);
}

void Plugin::Log(spdlog::level::level_enum level, char* msg) const
{
	eLogLevel eLevel = eLogLevel::LOG_INFO;

	switch (level)
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

	PluginMsg(eLevel, m_name.c_str(), "%s\n", msg);
}

bool Plugin::IsValid() const
{
	return m_valid && m_pCreateInterface && m_pluginId && m_callbacks && m_handle;
}

const std::string& Plugin::GetName() const
{
	return m_name;
}

const std::string& Plugin::GetLogName() const
{
	return m_logName;
}

const std::string& Plugin::GetDependencyName() const
{
	return m_dependencyName;
}

const std::string& Plugin::GetLocation() const
{
	return m_location;
}

bool Plugin::ShouldRunOnServer() const
{
	return m_runOnServer;
}

bool Plugin::ShouldRunOnClient() const
{
	return m_runOnClient;
}

void* Plugin::CreateInterface(const char* name, int* status) const
{
	return m_pCreateInterface(name, status);
}

void Plugin::Init(bool reloaded) const
{
	m_callbacks->Init(g_NorthstarModule, &m_initData, reloaded);
}

void Plugin::Finalize() const
{
	m_callbacks->Finalize();
}

void Plugin::OnSqvmCreated(CSquirrelVM* sqvm) const
{
	m_callbacks->OnSqvmCreated(sqvm);
}

void Plugin::OnSqvmDestroying(CSquirrelVM* sqvm) const
{
	DevMsg(eLog::PLUGSYS, "destroying sqvm %i\n", sqvm->vmContext);
	m_callbacks->OnSqvmDestroying(sqvm);
}

void Plugin::OnLibraryLoaded(HMODULE module, const char* name) const
{
	m_callbacks->OnLibraryLoaded(module, name);
}

void Plugin::RunFrame() const
{
	m_callbacks->RunFrame();
}
