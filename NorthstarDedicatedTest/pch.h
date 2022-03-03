#ifndef PCH_H
#define PCH_H

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define RAPIDJSON_NOMEMBERITERATORCLASS // need this for rapidjson
#define NOMINMAX						// this too
#define _WINSOCK_DEPRECATED_NO_WARNINGS // temp because i'm very lazy and want to use inet_addr, remove later
#define RAPIDJSON_HAS_STDSTRING 1

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

// httplib ssl

// add headers that you want to pre-compile here
#include "memalloc.h"

#include <Windows.h>
#include <Psapi.h>
#include <comdef.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <regex>
#include <thread>
#include <set>

#include "logging.h"
#include "include/MinHook.h"
#include "spdlog/spdlog.h"
#include "libcurl/include/curl/curl.h"
#include "hookutils.h"

template <typename ReturnType, typename... Args> ReturnType CallVFunc(int index, void* thisPtr, Args... args)
{
	return (*reinterpret_cast<ReturnType(__fastcall***)(void*, Args...)>(thisPtr))[index](thisPtr, args...);
}

#endif