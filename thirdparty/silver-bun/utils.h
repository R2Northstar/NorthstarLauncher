#pragma once

#ifndef USE_PRECOMPILED_HEADERS
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>
#include <psapi.h>
#else
#pragma message("ADD PRECOMPILED HEADERS TO SILVER-BUN.")
#endif // !USE_PRECOMPILED_HEADERS

namespace Utils
{
    std::vector<int> PatternToBytes(const char* szInput);
    std::pair<std::vector<uint8_t>, std::string> PatternToMaskedBytes(const char* szInput);
    std::vector<int> StringToBytes(const char* szInput, bool bNullTerminator);
    std::pair<std::vector<uint8_t>, std::string> StringToMaskedBytes(const char* szInput, bool bNullTerminator);
}

typedef const unsigned char* rsig_t;