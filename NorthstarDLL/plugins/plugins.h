#pragma once
#include <stdint.h>
#include "plugin_abi.h"

const int IDR_RCDATA1 = 101;

class Plugin
{
  public:
	const bool valid;

	const char* name;
	const char* logName;
	const char* dependencyName;
	const char* description;

	const uint32_t api_version;
	const char* version;

	// For now this is just implemented as the index into the plugins array
	// Maybe a bit shit but it works
	const int handle;

	std::shared_ptr<ColoredLogger> logger;

	const uint8_t runOnContext; // bitfield that gets resolved to run_on_client and run_on_server
	const bool run_on_client = false;
	const bool run_on_server = false;

  public:
	Plugin(HMODULE lib, int handle, std::string path);
	/*
	Plugin(
		const bool valid, // true if all required values are not null

		const char* name,
		const char* logName,
		const char* dependencyName,
		const char* description,
		uint32_t api_version,
		const char* version,
		int handle,
		bool runOnServer,
		bool runOnClient,
		const PLUGIN_INIT_TYPE init,
		const PLUGIN_INIT_SQVM_TYPE initClientSqvm,
		const PLUGIN_INIT_SQVM_TYPE initServerSqvm,
		const PLUGIN_INFORM_SQVM_CREATED_TYPE sqvmCreated,
		const PLUGIN_INFORM_SQVM_DESTROYED_TYPE sqvmDestroyed,
		const PLUGIN_INFORM_DLL_LOAD_TYPE informLibLoad,
		const PLUGIN_RUNFRAME runFrame)
		: name(name), logName(logName), dependencyName(dependencyName), description(description), api_version(api_version),
		  version(version), handle(handle), run_on_client(runOnClient), run_on_server(runOnServer), init(init),
		  init_sqvm_client(initClientSqvm), init_sqvm_server(initServerSqvm), inform_sqvm_created(sqvmCreated),
		  inform_sqvm_destroyed(sqvmDestroyed), inform_dll_load(informLibLoad), run_frame(runFrame) {};
		  */

	const PLUGIN_INIT_TYPE init;

	// all following functions are optional. Maybe should be std::optional in the future
	const PLUGIN_INIT_SQVM_TYPE init_sqvm_client;
	const PLUGIN_INIT_SQVM_TYPE init_sqvm_server;
	const PLUGIN_INFORM_SQVM_CREATED_TYPE inform_sqvm_created;
	const PLUGIN_INFORM_SQVM_DESTROYED_TYPE inform_sqvm_destroyed;

	const PLUGIN_INFORM_DLL_LOAD_TYPE inform_dll_load;

	const PLUGIN_RUNFRAME run_frame;
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
