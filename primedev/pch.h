#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here
#include "core/memalloc.h"

#include <windows.h>
#include <psapi.h>
#include <set>
#include <map>
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

// clang-format off
#define assert_msg(exp, msg) assert((exp, msg))
//clang-format on

#define NOTE_UNUSED(var) do { (void)var; } while(false)

#include "core/macros.h"

#include "core/math/color.h"

#include "spdlog/spdlog.h"
#include "logging/logging.h"
#include "MinHook.h"
#include "curl/curl.h"
#include "silver-bun/module.h"
#include "silver-bun/memaddr.h"
#include "core/hooks.h"

#endif
