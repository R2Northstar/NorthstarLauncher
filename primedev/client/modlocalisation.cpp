#include "mods/modmanager.h"

AUTOHOOK_INIT()

void* g_pVguiLocalize;

static bool(__fastcall* o_pCLocalise__AddFile)(
	void* pVguiLocalize, const char* path, const char* pathId, bool bIncludeFallbackSearchPaths) = nullptr;
static bool __fastcall h_CLocalise__AddFile(void* pVguiLocalize, const char* path, const char* pathId, bool bIncludeFallbackSearchPaths)
{
	// save this for later
	g_pVguiLocalize = pVguiLocalize;

	bool ret = o_pCLocalise__AddFile(pVguiLocalize, path, pathId, bIncludeFallbackSearchPaths);
	if (ret)
		spdlog::info("Loaded localisation file {} successfully", path);

	return true;
}

// clang-format off
AUTOHOOK(CLocalize__ReloadLocalizationFiles, localize.dll + 0xB830,
void, __fastcall, (void* pVguiLocalize))
// clang-format on
{
	// load all mod localization manually, so we keep track of all files, not just previously loaded ones
	for (Mod mod : g_pModManager->m_LoadedMods)
		if (mod.m_bEnabled)
			for (std::string& localisationFile : mod.LocalisationFiles)
				o_pCLocalise__AddFile(g_pVguiLocalize, localisationFile.c_str(), nullptr, false);

	spdlog::info("reloading localization...");
	CLocalize__ReloadLocalizationFiles(pVguiLocalize);
}

// clang-format off
AUTOHOOK(CEngineVGui__Init, engine.dll + 0x247E10,
void, __fastcall, (void* self))
// clang-format on
{
	CEngineVGui__Init(self); // this loads r1_english, valve_english, dev_english

	// previously we did this in CLocalize::AddFile, but for some reason it won't properly overwrite localization from
	// files loaded previously if done there, very weird but this works so whatever
	for (Mod mod : g_pModManager->m_LoadedMods)
		if (mod.m_bEnabled)
			for (std::string& localisationFile : mod.LocalisationFiles)
				o_pCLocalise__AddFile(g_pVguiLocalize, localisationFile.c_str(), nullptr, false);
}

ON_DLL_LOAD_CLIENT("localize.dll", Localize, (CModule module))
{
	AUTOHOOK_DISPATCH()
	o_pCLocalise__AddFile = module.Offset(0x6D80).RCast<decltype(o_pCLocalise__AddFile)>();
	HookAttach(&(PVOID&)o_pCLocalise__AddFile, (PVOID)h_CLocalise__AddFile);
}
