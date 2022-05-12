#pragma once

// use the R2 namespace for game funcs
namespace R2
{
	typedef const char* (*GetCurrentPlaylistNameType)();
	extern GetCurrentPlaylistNameType GetCurrentPlaylistName;

	typedef void (*SetCurrentPlaylistType)(const char* playlistName);
	extern SetCurrentPlaylistType SetCurrentPlaylist;

	typedef void (*SetPlaylistVarOverrideType)(const char* varName, const char* value);
	extern SetPlaylistVarOverrideType SetPlaylistVarOverride;

	typedef char* (*GetCurrentPlaylistVarType)(const char* varName, bool useOverrides);
	extern GetCurrentPlaylistVarType GetCurrentPlaylistVar;
} // namespace R2
