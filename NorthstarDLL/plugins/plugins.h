#pragma once
#include <stdint.h>
#include "plugin_abi.h"

const int IDR_RCDATA1 = 101;

// can't use bitwise ops on enum classes
namespace PluginContext {
	enum: uint8_t
		  {
			  DEDICATED = 0x1,
			  CLIENT = 0x2,
		  };
}

class Plugin
{
  public:
	char* name;
	char* displayName;
	char* dependencyName;
	char* description;

	uint32_t api_version;
	char* version;

	// For now this is just implemented as the index into the plugins array
	// Maybe a bit shit but it works
	int handle;

	std::shared_ptr<ColoredLogger> logger;

	bool run_on_client = false;
	bool run_on_server = false;

  public:
	PLUGIN_INIT_TYPE init;
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
	bool LoadPlugins();
	std::optional<Plugin> LoadPlugin(fs::path path, PluginInitFuncs* funcs, PluginNorthstarData* data);

	void InformSQVMLoad(ScriptContext context, SquirrelFunctions* s);
	void InformSQVMCreated(ScriptContext context, CSquirrelVM* sqvm);
	void InformSQVMDestroyed(ScriptContext context);

	void InformDLLLoad(const char* dll, void* data, void* dllPtr);

	void RunFrame();

  private:
	std::string pluginPath;
};

extern PluginManager* g_pPluginManager;
