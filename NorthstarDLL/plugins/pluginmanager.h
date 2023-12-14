#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include <windows.h>

class Plugin;

class PluginManager
{
  public:
	std::vector<Plugin> GetLoadedPlugins();
	std::optional<Plugin> GetPlugin(int handle); // get a plugin by it's handle
	int GetNewHandle(); // get an available handle for a new plugin
	bool LoadPlugins();
	void LoadPlugin(fs::path path);

	// callback triggers
	void InformSQVMCreated(CSquirrelVM* sqvm);
	void InformSQVMDestroyed(ScriptContext context);
	void InformDllLoad(HMODULE module, fs::path path);
	void RunFrame();

  private:
	void InformAllPluginsInitialized();

	std::vector<Plugin> m_vLoadedPlugins;
	std::string pluginPath;
};

extern PluginManager* g_pPluginManager;

#endif
