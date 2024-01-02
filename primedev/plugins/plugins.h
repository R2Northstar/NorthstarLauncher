#pragma once
#include <stdint.h>
#include "core/sourceinterface.h"
#include "plugins/interfaces/interface.h"
#include "plugins/interfaces/IPluginId.h"
#include "plugins/interfaces/IPluginCallbacks.h"

class Plugin
{
private:
	CreateInterfaceFn m_pCreateInterface;
	IPluginId* pluginId = 0;
	IPluginCallbacks* callbacks = 0;

	std::shared_ptr<ColoredLogger> logger;

	bool valid = false;
	std::string name;
	std::string logName;
	std::string dependencyName;
	bool runOnServer;
	bool runOnClient;

public:
	Plugin(std::string path);
	bool Unload();
	void Reload();

	// sys
	void Log(spdlog::level::level_enum level, char* msg);

	// callbacks
	bool IsValid() const;
	std::string GetName() const;
	std::string GetLogName() const;
	std::string GetDependencyName() const;
	bool ShouldRunOnServer() const;
	bool ShouldRunOnClient() const;
	void* CreateInterface(const char* pName, int* pStatus) const;
	void Init(bool reloaded);
	void Finalize();
	void OnSqvmCreated(CSquirrelVM* sqvm);
	void OnSqvmDestroying(CSquirrelVM* sqvm);
	void OnLibraryLoaded(HMODULE module, const char* name);
	void RunFrame();

	HMODULE handle;
	std::string location; // path of the dll
	PluginNorthstarData initData;
};
