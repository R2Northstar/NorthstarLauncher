#pragma once
#include <stdint.h>
#include "plugin_abi.h"
#include "core/sourceinterface.h"
#include "plugins/interfaces/interface.h"
#include "plugins/interfaces/IPluginId.h"
#include "plugins/interfaces/IPluginCallbacks.h"

const int IDR_RCDATA1 = 101;

class Plugin
{
  private:
	CreateInterfaceFn m_pCreateInterface;
	IPluginId* pluginId;
	IPluginCallbacks* callbacks;

	HMODULE module = nullptr;
	std::shared_ptr<ColoredLogger> logger;

	bool valid = false;
	std::string name;
	std::string logName;
	std::string dependencyName;
	bool runOnServer;
	bool runOnClient;

  public:
	Plugin(std::string path);

	void Unload();
	void Log(spdlog::level::level_enum level, char* msg)
	{
		logger->log(level, msg);
	}

	const bool IsValid() const
	{
		return valid;
	};

	std::string GetName() const
	{
		return name;
	};

	std::string GetLogName() const
	{
		return logName;
	};

	std::string GetDependencyName() const
	{
		return dependencyName;
	};

	bool ShouldRunOnServer()
	{
		return this->runOnServer;
	}

	bool ShouldRunOnClient()
	{
		return this->runOnClient;
	}

	void* CreateInterface(const char* pName, int* pStatus) const
	{
		return m_pCreateInterface(pName, pStatus);
	}

	void Init()
	{
		this->callbacks->Init(&this->initData);
	};

	void Finalize()
	{
		this->callbacks->Finalize();
	};

	const int handle; // identifier of this plugin used only for logging atm
	const std::string location; // path of the dll
	const PluginNorthstarData initData;

	PLUGIN_INIT_TYPE init;

	// all following functions are optional. Maybe should be std::optional in the future
	PLUGIN_INIT_SQVM_TYPE init_sqvm_client;
	PLUGIN_INIT_SQVM_TYPE init_sqvm_server;
	PLUGIN_INFORM_SQVM_CREATED_TYPE inform_sqvm_created;
	PLUGIN_INFORM_SQVM_DESTROYED_TYPE inform_sqvm_destroyed;

	PLUGIN_INFORM_DLL_LOAD_TYPE inform_dll_load;

	PLUGIN_RUNFRAME run_frame;
};

class PluginManager
{
  public:
	std::vector<Plugin> m_vLoadedPlugins;

  public:
	std::optional<Plugin*> GetPlugin(int handle);
	int GetNewHandle();
	bool LoadPlugins();
	std::optional<Plugin> LoadPlugin(fs::path path);

	void InformAllPluginsInitialized();

	void InformSQVMLoad(ScriptContext context, SquirrelFunctions* s);
	void InformSQVMCreated(ScriptContext context, CSquirrelVM* sqvm);
	void InformSQVMDestroyed(ScriptContext context);

	void InformDLLLoad(const char* dll, void* data, void* dllPtr);

	void RunFrame();

  private:
	std::string pluginPath;
};

extern PluginManager* g_pPluginManager;
