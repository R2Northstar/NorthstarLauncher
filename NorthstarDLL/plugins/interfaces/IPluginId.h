#ifndef IPLUGIN_ID_H
#define IPLUGIN_ID_H

#include <stdint.h>
#include "squirrel/squirrelclasstypes.h"

#define PLUGIN_ID_VERSION "PluginId001"

/*
struct IPluginId {
	struct {
		void* (*GetProperty)(IPluginId* self, int prop);
	}* vftable;
};
*/

// an identifier for the type of string data requested from the plugin
enum class PluginString
{
	NAME = 0,
	LOG_NAME = 1,
	DEPENDENCY_NAME = 2,
	CONTEXT = 3,
};

// an identifier for the type of bitflag requested from the plugin
enum class PluginField
{
	CONTEXT = 0,
};

// an interface that is required from every plugin to query data about it
class IPluginId
{
  public:
	virtual const char* GetString(PluginString prop) = 0;
	virtual int64_t GetField(PluginField prop) = 0;
	virtual void OnSqvmCreated(CSquirrelVM* sqvm);
	virtual void OnSqvmDestroyed(CSquirrelVM* sqvm);
	virtual void OnLibraryLoaded();
};

#endif
