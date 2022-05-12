#include "pch.h"
#include "hooks.h"
#include "modlocalisation.h"
#include "hookutils.h"
#include "modmanager.h"


typedef bool (*AddLocalisationFileType)(void* g_pVguiLocalize, const char* path, const char* pathId, char unknown);
AddLocalisationFileType AddLocalisationFile;
bool AddLocalisationFileHook(void* g_pVguiLocalize, const char* path, const char* pathId, char unknown)
{
	static bool bLoadModLocalisationFiles = true;
	bool ret = AddLocalisationFile(g_pVguiLocalize, path, pathId, unknown);

	if (ret)
		spdlog::info("Loaded localisation file {} successfully", path);

	if (!bLoadModLocalisationFiles)
		return ret;

	bLoadModLocalisationFiles = false;

	for (Mod mod : g_pModManager->m_loadedMods)
		if (mod.Enabled)
			for (std::string& localisationFile : mod.LocalisationFiles)
				AddLocalisationFile(g_pVguiLocalize, localisationFile.c_str(), pathId, unknown);

	bLoadModLocalisationFiles = true;

	return ret;
}

ON_DLL_LOAD_CLIENT("localize.dll", Localize, [](HMODULE baseAddress)
{
	HookEnabler hook;
	ENABLER_CREATEHOOK(hook, (char*)baseAddress + 0x6D80, AddLocalisationFileHook, reinterpret_cast<LPVOID*>(&AddLocalisationFile));
})