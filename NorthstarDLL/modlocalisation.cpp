#include "pch.h"
#include "modmanager.h"

AUTOHOOK_INIT()

AUTOHOOK(AddLocalisationFile, localize.dll + 0x6D80,
bool,, (void* pVguiLocalize, const char* path, const char* pathId, char unknown))
{
	static bool bLoadModLocalisationFiles = true;
	bool ret = AddLocalisationFile(pVguiLocalize, path, pathId, unknown);

	if (ret)
		spdlog::info("Loaded localisation file {} successfully", path);

	if (!bLoadModLocalisationFiles)
		return ret;

	bLoadModLocalisationFiles = false;

	for (Mod mod : g_pModManager->m_LoadedMods)
		if (mod.m_bEnabled)
			for (std::string& localisationFile : mod.LocalisationFiles)
				AddLocalisationFile(pVguiLocalize, localisationFile.c_str(), pathId, unknown);

	bLoadModLocalisationFiles = true;

	return ret;
}

ON_DLL_LOAD_CLIENT("localize.dll", Localize, (HMODULE baseAddress))
{
	AUTOHOOK_DISPATCH()
}