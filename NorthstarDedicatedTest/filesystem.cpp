#include "pch.h"
#include "filesystem.h"
#include "hooks.h"
#include "hookutils.h"
#include "sourceinterface.h"
#include "modmanager.h"

#include <iostream>
#include <sstream>

using namespace R2FS;

bool bReadingOriginalFile = false;
std::string sCurrentModPath;

ConVar* Cvar_ns_fs_log_reads;

namespace R2FS
{
	std::string ReadVPKFile(const char* path)
	{
		// read scripts.rson file, todo: check if this can be overwritten
		FileHandle_t fileHandle = (*g_pFilesystem)->m_vtable2->Open(&(*g_pFilesystem)->m_vtable2, path, "rb", "GAME", 0);

		std::stringstream fileStream;
		int bytesRead = 0;
		char data[4096];
		do
		{
			bytesRead = (*g_pFilesystem)->m_vtable2->Read(&(*g_pFilesystem)->m_vtable2, data, (int)std::size(data), fileHandle);
			fileStream.write(data, bytesRead);
		} while (bytesRead == std::size(data));

		(*g_pFilesystem)->m_vtable2->Close(*g_pFilesystem, fileHandle);

		return fileStream.str();
	}

	std::string ReadVPKOriginalFile(const char* path)
	{
		bReadingOriginalFile = true;
		std::string ret = ReadVPKFile(path);
		bReadingOriginalFile = false;

		return ret;
	}

	SourceInterface<IFileSystem>* g_pFilesystem;
}

typedef void (*AddSearchPathType)(IFileSystem* fileSystem, const char* pPath, const char* pathID, SearchPathAdd_t addType);
AddSearchPathType AddSearchPath;
void AddSearchPathHook(IFileSystem* fileSystem, const char* pPath, const char* pathID, SearchPathAdd_t addType)
{
	AddSearchPath(fileSystem, pPath, pathID, addType);

	// make sure current mod paths are at head
	if (!strcmp(pathID, "GAME") && sCurrentModPath.compare(pPath) && addType == PATH_ADD_TO_HEAD)
	{
		AddSearchPath(fileSystem, sCurrentModPath.c_str(), "GAME", PATH_ADD_TO_HEAD);
		AddSearchPath(fileSystem, GetCompiledAssetsPath().string().c_str(), "GAME", PATH_ADD_TO_HEAD);
	}
}

void SetNewModSearchPaths(Mod* mod)
{
	// put our new path to the head if we need to read from a different mod path
	// in the future we could also determine whether the file we're setting paths for needs a mod dir, or compiled assets
	if (mod != nullptr)
	{
		if ((fs::absolute(mod->ModDirectory) / MOD_OVERRIDE_DIR).string().compare(sCurrentModPath))
		{
			spdlog::info("changing mod search path from {} to {}", sCurrentModPath, mod->ModDirectory.string());

			AddSearchPath(
				&*(*g_pFilesystem), (fs::absolute(mod->ModDirectory) / MOD_OVERRIDE_DIR).string().c_str(), "GAME", PATH_ADD_TO_HEAD);
			sCurrentModPath = (fs::absolute(mod->ModDirectory) / MOD_OVERRIDE_DIR).string();
		}
	}
	else // push compiled to head
		AddSearchPath(&*(*g_pFilesystem), fs::absolute(GetCompiledAssetsPath()).string().c_str(), "GAME", PATH_ADD_TO_HEAD);
}

bool TryReplaceFile(const char* pPath, bool shouldCompile)
{
	if (bReadingOriginalFile)
		return false;

	if (shouldCompile)
		g_ModManager->CompileAssetsForFile(pPath);

	// idk how efficient the lexically normal check is
	// can't just set all /s in path to \, since some paths aren't in writeable memory
	auto file = g_ModManager->m_modFiles.find(g_ModManager->NormaliseModFilePath(fs::path(pPath)));
	if (file != g_ModManager->m_modFiles.end())
	{
		SetNewModSearchPaths(file->second.owningMod);
		return true;
	}

	return false;
}

// force modded files to be read from mods, not cache
typedef bool (*ReadFromCacheType)(IFileSystem* filesystem, char* path, void* result);
ReadFromCacheType ReadFromCache;
bool ReadFromCacheHook(IFileSystem* filesystem, char* pPath, void* result)
{
	if (TryReplaceFile(pPath, true))
		return false;

	return ReadFromCache(filesystem, pPath, result);
}

// force modded files to be read from mods, not vpk
typedef FileHandle_t (*ReadFileFromVPKType)(VPKData* vpkInfo, __int64* b, char* filename);
ReadFileFromVPKType ReadFileFromVPK;
FileHandle_t ReadFileFromVPKHook(VPKData* vpkInfo, __int64* b, char* filename)
{
	// don't compile here because this is only ever called from OpenEx, which already compiles
	if (TryReplaceFile(filename, false))
	{
		*b = -1;
		return b;
	}

	return ReadFileFromVPK(vpkInfo, b, filename);
} 

typedef FileHandle_t (*CBaseFileSystem__OpenExType)(
	IFileSystem* filesystem, const char* pPath, const char* pOptions, uint32_t flags, const char* pPathID, char** ppszResolvedFilename);
CBaseFileSystem__OpenExType CBaseFileSystem__OpenEx;
FileHandle_t CBaseFileSystem__OpenExHook(IFileSystem* filesystem, const char* pPath, const char* pOptions, uint32_t flags, const char* pPathID, char **ppszResolvedFilename)
{
	TryReplaceFile(pPath, true);
	return CBaseFileSystem__OpenEx(filesystem, pPath, pOptions, flags, pPathID, ppszResolvedFilename);
}

typedef VPKData* (*MountVPKType)(IFileSystem* fileSystem, const char* vpkPath);
MountVPKType MountVPK;
VPKData* MountVPKHook(IFileSystem* fileSystem, const char* vpkPath)
{
	spdlog::info("MountVPK {}", vpkPath);
	VPKData* ret = MountVPK(fileSystem, vpkPath);

	for (Mod mod : g_ModManager->m_loadedMods)
	{
		if (!mod.Enabled)
			continue;

		for (ModVPKEntry& vpkEntry : mod.Vpks)
		{
			// if we're autoloading, just load no matter what
			if (!vpkEntry.m_bAutoLoad)
			{
				// resolve vpk name and try to load one with the same name
				// todo: we should be unloading these on map unload manually
				std::string mapName(fs::path(vpkPath).filename().string());
				std::string modMapName(fs::path(vpkEntry.m_sVpkPath.c_str()).filename().string());
				if (mapName.compare(modMapName))
					continue;
			}

			VPKData* loaded = MountVPK(fileSystem, vpkEntry.m_sVpkPath.c_str());
			if (!ret) // this is primarily for map vpks and stuff, so the map's vpk is what gets returned from here
				ret = loaded;
		}
	}

	return ret;
}

ON_DLL_LOAD("filesystem_stdio.dll", Filesystem, [](HMODULE baseAddress)
{
	R2FS::g_pFilesystem = new SourceInterface<IFileSystem>("filesystem_stdio.dll", "VFileSystem017");

	// create hooks
	HookEnabler hook;
	ENABLER_CREATEHOOK(hook, (*g_pFilesystem)->m_vtable->ReadFromCache, &ReadFromCacheHook, reinterpret_cast<LPVOID*>(&ReadFromCache));
	ENABLER_CREATEHOOK(hook, (char*)baseAddress + 0x5CBA0, &ReadFileFromVPKHook, reinterpret_cast<LPVOID*>(&ReadFileFromVPK));
	ENABLER_CREATEHOOK(
		hook, (char*)baseAddress + 0x15F50, &CBaseFileSystem__OpenExHook, reinterpret_cast<LPVOID*>(&CBaseFileSystem__OpenEx));
	ENABLER_CREATEHOOK(hook, (*g_pFilesystem)->m_vtable->AddSearchPath, &AddSearchPathHook, reinterpret_cast<LPVOID*>(&AddSearchPath));
	ENABLER_CREATEHOOK(hook, (*g_pFilesystem)->m_vtable->MountVPK, &MountVPKHook, reinterpret_cast<LPVOID*>(&MountVPK));
})