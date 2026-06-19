#include "core/tier1.h"
#include "plugins/interfaces/interface.h"
#include "IPluginHooks.h"
#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

static std::unordered_map<HMODULE, std::vector<std::string>> registeredHooks = {};
static std::unordered_map<HMODULE, RegisterHookCallback> hooksCallback = {};

std::optional<HMODULE> ReturnAddressToHMODULE(const char* sourceName)
{
	void* callee = _ReturnAddress();
	void* pluginHandle = 0;
	RtlPcToFileHeader(callee, &pluginHandle);

	// This should never happen
	if (!pluginHandle)
	{
		NS::log::PLUGINSYS->warn(
			"Attempted to call {} from an unmapped plugin from callee {}", sourceName, static_cast<void*>(pluginHandle));
		return nullptr;
	}

	return std::make_optional((HMODULE)pluginHandle);
}

class CPluginHooks : public IPluginHooks
{
public:
	void RegisterCallback(RegisterHookCallback callback)
	{
		auto maybePlugin = ReturnAddressToHMODULE(__FUNCTION__);
		if (!maybePlugin.has_value())
			return;
		HMODULE plugin = maybePlugin.value();

		hooksCallback.insert_or_assign(plugin, callback);
	}
	void RegisterHook(const char* hookName)
	{
		auto maybePlugin = ReturnAddressToHMODULE(__FUNCTION__);
		if (!maybePlugin.has_value())
			return;
		HMODULE plugin = maybePlugin.value();

		if (!registeredHooks.contains(plugin))
		{
			registeredHooks[plugin] = {};
		}

		registeredHooks[plugin].push_back(std::string(hookName));
	}

	void DropHooks()
	{
		auto maybePlugin = ReturnAddressToHMODULE(__FUNCTION__);
		if (!maybePlugin.has_value())
			return;
		HMODULE plugin = maybePlugin.value();

		registeredHooks.erase(plugin);
		hooksCallback.erase(plugin);
	}

	RegisterHookCallback GetHookCallback(const char* hookName)
	{
		std::optional<HMODULE> maybePlugin = nullptr;
		for (auto val : registeredHooks)
		{
			if (std::find_if(val.second.begin(), val.second.end(), [hookName](std::string name) { return name == hookName; }) !=
				val.second.end())
			{
				maybePlugin = val.first;
				break;
			}
		}
		if (!maybePlugin.has_value())
			return nullptr;
		HMODULE plugin = maybePlugin.value();

		return hooksCallback.contains(plugin) ? hooksCallback[plugin] : nullptr;
	}
};

EXPOSE_SINGLE_INTERFACE(CPluginHooks, IPluginHooks, PLUGINS_HOOKS_VERSION);
