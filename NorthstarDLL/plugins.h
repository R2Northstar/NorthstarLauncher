#pragma once
#include "plugin_abi.h"

class Plugin
{
  public:
	std::string name;
	std::string displayName;
	std::string description;

	std::string api_version;
	std::string version;

	bool run_on_client = false;
	bool run_on_server = false;

  public:
	PLUGIN_INIT_TYPE init;
	PLUGIN_INIT_SQVM_TYPE init_sqvm_client;
	PLUGIN_INIT_SQVM_TYPE init_sqvm_server;
	PLUGIN_INFORM_SQVM_CREATED_TYPE inform_sqvm_created;
};

class PluginManager
{
  public:
	std::vector<Plugin> m_vLoadedPlugins;

  public:
	bool LoadPlugins();
	std::optional<Plugin> LoadPlugin(fs::path path, PluginInitFuncs* funcs);

	void InformSQVMLoad(ScriptContext context, SquirrelFunctions* s);
	void InformSQVMCreated(ScriptContext context, CSquirrelVM* sqvm);

  private:
	std::string pluginPath;
};

extern PluginManager* g_pPluginManager;

void initGameState();
void* getPluginObject(PluginObject var);
