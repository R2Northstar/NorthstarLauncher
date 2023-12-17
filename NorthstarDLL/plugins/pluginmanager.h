#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include <windows.h>

class Plugin;

class PluginManager
{
  public:
	std::vector<Plugin> GetLoadedPlugins();
	std::optional<Plugin> GetPlugin(HMODULE handle);
	bool LoadPlugins(bool reloaded = false);
	void LoadPlugin(fs::path path, bool reloaded = false);
	void ReloadPlugins();
	void RemovePlugin(HMODULE handle);

	// callback triggers
	void InformSqvmCreated(CSquirrelVM* sqvm);
	void InformSqvmDestroying(CSquirrelVM* sqvm);
	void InformDllLoad(HMODULE module, fs::path path);
	void RunFrame();

  private:
	void InformAllPluginsInitialized();

	std::vector<Plugin> plugins;
	std::string pluginPath;
};

extern PluginManager* g_pPluginManager;

#endif
