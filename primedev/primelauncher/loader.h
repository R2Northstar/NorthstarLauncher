#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

extern wchar_t exePath[4096];
extern wchar_t buffer1[8192];
extern wchar_t buffer2[12288];

void LibraryLoadError(DWORD dwMessageId, const wchar_t* libName, const wchar_t* location);
bool ShouldLoadNorthstar(bool default_case);
bool LoadNorthstar();
bool GetExePathWide(wchar_t* dest, DWORD destSize);
