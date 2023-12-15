#include "pluginmanager.h"

#include <regex>
#include "plugins.h"
#include "config/profile.h"

PluginManager* g_pPluginManager;

std::vector<Plugin> PluginManager::GetLoadedPlugins()
{
	return this->m_vLoadedPlugins;
}

std::optional<Plugin> PluginManager::GetPlugin(int handle)
{
	if (handle < 0 || handle >= this->m_vLoadedPlugins.size())
		return std::nullopt;
	return this->m_vLoadedPlugins[handle];
}

void PluginManager::LoadPlugin(fs::path path)
{
	Plugin plugin = Plugin(path.string());

	if (!plugin.IsValid())
	{
		NS::log::PLUGINSYS->warn("Unloading invalid plugin '{}'", path.string());
		plugin.Unload();
	}

	m_vLoadedPlugins.push_back(plugin);
	plugin.Init();
}

inline void FindPlugins(fs::path pluginPath, std::vector<fs::path>& paths)
{
	// ensure dirs exist
	if (!fs::exists(pluginPath) || !fs::is_directory(pluginPath))
	{
		return;
	}

	for (const fs::directory_entry& entry : fs::directory_iterator(pluginPath))
	{
		if (fs::is_regular_file(entry) && entry.path().extension() == ".dll")
			paths.emplace_back(entry.path());
	}
}

bool PluginManager::LoadPlugins()
{
	if (strstr(GetCommandLineA(), "-noplugins") != NULL)
	{
		NS::log::PLUGINSYS->warn("-noplugins detected; skipping loading plugins");
		return false;
	}

	fs::create_directories(GetThunderstoreModFolderPath());

	std::vector<fs::path> paths;

	pluginPath = GetNorthstarPrefix() + "\\plugins";

	fs::path libPath = fs::absolute(pluginPath + "\\lib");
	if (fs::exists(libPath) && fs::is_directory(libPath))
		AddDllDirectory(libPath.wstring().c_str());

	FindPlugins(pluginPath, paths);

	// Special case for Thunderstore mods dir
	std::filesystem::directory_iterator thunderstoreModsDir = fs::directory_iterator(GetThunderstoreModFolderPath());
	// Set up regex for `AUTHOR-MOD-VERSION` pattern
	std::regex pattern(R"(.*\\([a-zA-Z0-9_]+)-([a-zA-Z0-9_]+)-(\d+\.\d+\.\d+))");
	for (fs::directory_entry dir : thunderstoreModsDir)
	{
		fs::path pluginsDir = dir.path() / "plugins";
		// Use regex to match `AUTHOR-MOD-VERSION` pattern
		if (!std::regex_match(dir.path().string(), pattern))
		{
			spdlog::warn("The following directory did not match 'AUTHOR-MOD-VERSION': {}", dir.path().string());
			continue; // skip loading package that doesn't match
		}

		fs::path libDir = fs::absolute(pluginsDir / "lib");
		if (fs::exists(libDir) && fs::is_directory(libDir))
			AddDllDirectory(libDir.wstring().c_str());

		FindPlugins(pluginsDir, paths);
	}

	if (paths.empty())
	{
		NS::log::PLUGINSYS->warn("Could not find any plugins. Skipped loading plugins");
		return false;
	}

	for (fs::path path : paths)
	{
		LoadPlugin(path);
	}

	InformAllPluginsInitialized();

	return true;
}

int PluginManager::GetNewHandle()
{
	return this->m_vLoadedPlugins.size();
}

void PluginManager::InformAllPluginsInitialized()
{
	for(Plugin& plugin : this->m_vLoadedPlugins)
	{
		plugin.Finalize();
	}
}

void PluginManager::InformSqvmCreated(CSquirrelVM* sqvm)
{
	for(Plugin& plugin : GetLoadedPlugins())
	{
		plugin.OnSqvmCreated(sqvm);
	}
}

void PluginManager::InformSqvmDestroying(CSquirrelVM* sqvm)
{
	for(Plugin& plugin : GetLoadedPlugins())
	{
		plugin.OnSqvmDestroying(sqvm);
	}
}

void PluginManager::InformDllLoad(HMODULE module, fs::path path)
{
	const char* filename = path.filename().string().c_str();
	for(Plugin& plugin : GetLoadedPlugins())
	{
		plugin.OnLibraryLoaded(module, filename);
	}
}

void PluginManager::RunFrame()
{
	for(Plugin& plugin : GetLoadedPlugins())
	{
		plugin.RunFrame();
	}
}
