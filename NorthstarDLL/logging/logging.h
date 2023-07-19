#pragma once
#include "spdlog/sinks/base_sink.h"
#include "spdlog/logger.h"
#include "squirrel/squirrel.h"
#include "core/math/color.h"

// Directory we put log files in ( %profile%\\logs\\%timestamp%\\ )
inline std::string g_svLogDirectory;

// Windows terminal logger
inline std::shared_ptr<spdlog::logger> g_WinLogger;

// Settings
inline bool g_bConsole_UseAnsiColor = false;
inline bool g_bSpdLog_CreateLogFiles = true;

// Log file settings
constexpr int SPDLOG_MAX_LOG_SIZE = 10 * 1024 * 1024; // 10 MB max
constexpr int SPDLOG_MAX_FILES = 512;

//-----------------------------------------------------------------------------
void SpdLog_PreInit(void);
void SpdLog_Init(void);
void SpdLog_CreateLoggers(void);
void SpdLog_Shutdown(void);

// TODO [Fifty]: Possibly move into it's own file
void Console_Init(void);
void Console_PostInit(void);
void Console_Shutdown(void);
void StartupLog();
