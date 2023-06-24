#pragma once

//-----------------------------------------------------------------------------
// Filesystem helpers
bool FileExists(const fs::path& filePath);
bool PathIsAbsolute(const fs::path& filePath);
bool PathIsRelative(const fs::path& filePath);

bool CreateDirectory(const fs::path& directory);

//-----------------------------------------------------------------------------
// String helpers
std::string FormatV(const char* pszFormat, va_list vArgs);
std::string Format(const char* pszFormat, ...);

void RemoveAsciiControlSequences(char* str, bool allow_color_codes);
