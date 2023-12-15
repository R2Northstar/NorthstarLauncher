#ifndef IPLUGIN_CALLBACKS_H
#define IPLUGIN_CALLBACKS_H

#include <stdint.h>
#include "squirrel/squirrel.h"

// can't use bitwise ops on enum classes but I don't want these in the global namespace (user defined operators suck)
namespace PluginContext
{
	enum : uint8_t
	{
		DEDICATED = 0x1,
		CLIENT = 0x2,
	};
}

struct PluginNorthstarData
{
	const int pluginHandle;
};

class IPluginCallbacks
{
	public:
	virtual void Init(HMODULE northstarModule, const PluginNorthstarData* initData) = 0; // runs after the plugin is loaded and validated
	virtual void Finalize() = 0; // runs after all plugins have been loaded
	virtual void Unload() = 0; // runs just before the library is freed
	virtual void OnSqvmCreated(CSquirrelVM* sqvm) = 0;
	virtual void OnSqvmDestroying(CSquirrelVM* sqvm) = 0;
	virtual void OnLibraryLoaded(HMODULE module, const char* name) = 0;
	virtual void RunFrame() = 0;
};

#endif
