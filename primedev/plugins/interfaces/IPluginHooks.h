#ifndef ILOGGING_H
#define ILOGGING_H

#include <stdint.h>

#define PLUGINS_HOOKS_VERSION "NSPluginsHooks001"

typedef bool (*RegisterHookCallback)(const char* hookName, const void* data);

// unstable api for now
class IPluginHooks
{
public:
	virtual void RegisterCallback(RegisterHookCallback callback) = 0;
	virtual void RegisterHook(const char* hookName) = 0;
	virtual void DropHooks() = 0;
	virtual RegisterHookCallback GetHookCallback(const char* hookName) = 0;
};

#endif
