#ifndef IPLUGIN_CALLBACKS_H
#define IPLUGIN_CALLBACKS_H

#include "squirrel/squirrel.h"
#include "plugins/plugin_abi.h"

class IPluginCallbacks
{
	public:
	virtual void Init(const PluginNorthstarData* initData) = 0; // runs after the plugin is loaded and validated
	virtual void Finalize() = 0; // runs after all plugins have been loaded
	virtual void OnSqvmCreated(CSquirrelVM* sqvm) = 0;
	virtual void OnSqvmDestroyed(ScriptContext context) = 0;
	virtual void OnLibraryLoaded(HMODULE module, const char* name) = 0;
	virtual void RunFrame() = 0;
};

#endif
