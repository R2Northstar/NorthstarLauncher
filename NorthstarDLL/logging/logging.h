#pragma once
#include "spdlog/sinks/base_sink.h"
#include "spdlog/logger.h"
#include "squirrel/squirrel.h"
#include "core/math/color.h"

inline std::string g_LogDirectory;

inline std::shared_ptr<spdlog::logger> g_WinLogger;

void SpdLog_PreInit(void);
void SpdLog_Init(void);
void SpdLog_Shutdown(void);

// TODO [Fifty]: Possibly move into it's own file
void Console_Init(void);
void Console_Shutdown(void);
