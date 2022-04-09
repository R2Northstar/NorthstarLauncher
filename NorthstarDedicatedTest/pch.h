// clang-format off
#ifndef PCH_H
#define PCH_H

#ifdef _DEBUG
#define NSDEBUG
#endif

#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#define RAPIDJSON_NOMEMBERITERATORCLASS // need this for rapidjson
#define NOMINMAX						// this too
#define _WINSOCK_DEPRECATED_NO_WARNINGS // temp because i'm very lazy and want to use inet_addr, remove later
#define RAPIDJSON_HAS_STDSTRING 1

// httplib ssl

// add headers that you want to pre-compile here
#include "memalloc.h"

#include <Windows.h>
#include <Psapi.h>
#include <set>
#include <map>
#include <filesystem>
#include <sstream>

#include "logging.h"
#include "include/MinHook.h"
#include "spdlog/spdlog.h"
#include "libcurl/include/curl/curl.h"
#include "hookutils.h"

template <typename ReturnType, typename... Args> ReturnType CallVFunc(int index, void* thisPtr, Args... args)
{
	return (*reinterpret_cast<ReturnType(__fastcall***)(void*, Args...)>(thisPtr))[index](thisPtr, args...);
}

template <typename T, size_t index, typename... Args> constexpr T CallVFunc_Alt(void* classBase, Args... args) noexcept
{
	return ((*(T(__thiscall***)(void*, Args...))(classBase))[index])(classBase, args...);
}

#define STR_HASH(s) (std::hash<std::string>()(s))

// Example usage: M_VMETHOD(int, GetEntityIndex, 8, (CBaseEntity* ent), (this, ent))
#define M_VMETHOD(returnType, name, index, args, argsRaw)                                                                                  \
	FORCEINLINE returnType name args noexcept { return CallVFunc_Alt<returnType, index> argsRaw; }

// Logs a message only if in debug mode
#ifdef NS_DEBUG
#define DBLOG(s, ...) spdlog::info(s, ##__VA_ARGS__)
#else
#define DBLOG(s)
#endif

// Concatiations of some compile-time macros line __LINE__ require 2 levels of macro indirection
#define CONCAT_WRAP_INNER(a, b) a##b
#define CONCAT_WRAP(a, b) CONCAT_WRAP_INNER(a, b)

// Execute a block of code at startup runtime (before main)
struct _ORTC
{
	_ORTC(std::function<void()> f) { f(); }
};
#define RUNTIME_EXEC(lambdaFunc) const inline _ORTC CONCAT_WRAP_INNER(_ORTC_INSTANCE, __COUNTER__) = _ORTC(lambdaFunc)

// If-statement that only fires once
#define IF_ONCE                                                                                                                            \
	if ((                                                                                                                                  \
			[]                                                                                                                             \
			{                                                                                                                              \
				static bool _if_once_change_bool = true;                                                                                   \
				if (_if_once_change_bool)                                                                                                  \
				{                                                                                                                          \
					_if_once_change_bool = false;                                                                                          \
					return true;                                                                                                           \
				}                                                                                                                          \
				else                                                                                                                       \
					return false;                                                                                                          \
			})())

#endif

#define MAX(a, b) (a > b ? a : b)
#define MIN(a, b) (a < b ? a : b)