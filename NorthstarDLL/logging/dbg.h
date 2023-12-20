#pragma once

extern std::mutex g_LogMutex;

// We can only print to gameconsole when CEngineVgui is initilazed
inline bool g_bEngineVguiInitilazed = false;

//-----------------------------------------------------------------------------
// Log context
enum class eLogLevel : int
{
	LOG_INFO = 0,
	LOG_WARN = 1,
	LOG_ERROR = 2
};

//-----------------------------------------------------------------------------
// Log context
enum class eLog : int
{
	//-----------------------------------------------------------------------------
	// Common
	//-----------------------------------------------------------------------------
	NONE = 0,
	NS = 1,

	//-----------------------------------------------------------------------------
	// Script
	//-----------------------------------------------------------------------------
	SCRIPT_SERVER = 2,
	SCRIPT_CLIENT = 3,
	SCRIPT_UI = 4,

	//-----------------------------------------------------------------------------
	// Native
	//-----------------------------------------------------------------------------
	SERVER = 5,
	CLIENT = 6,
	UI = 7,
	ENGINE = 8,
	RTECH = 9,
	FS = 10,
	MAT = 11,
	AUDIO = 12,
	VIDEO = 13,

	//-----------------------------------------------------------------------------
	// Custom systems
	//-----------------------------------------------------------------------------
	MS = 14, // Masterserver
	MODSYS = 15, // Mod system
	PLUGSYS = 16, // Plugin system
	PLUGIN = 17,

	//-----------------------------------------------------------------------------
	// Misc
	//-----------------------------------------------------------------------------
	CHAT = 18 // Chat
};

// clang-format off
constexpr const char* sLogString[19] =
{
	"",
	"NORTHSTAR",
	"SCRIPT SV",
	"SCRIPT CL",
	"SCRIPT UI",
	"NATIVE SV",
	"NATIVE CL",
	"NATIVE UI",
	"NATIVE EN",
	"RTECH SYS",
	"FILESYSTM",
	"MATERIAL ",
	"AUDIO SYS",
	"VIDEO SYS",
	"MASTERSVR",
	"MODSYSTEM",
	"PLUGSYSTM",
	"PLUGIN", // This is replaced in PLUGIN_LOG
	"CHATSYSTM"
};

// clang-format on

void DevMsg(eLog eContext, const char* fmt, ...);
void Warning(eLog eContext, const char* fmt, ...);
void Error(eLog eContext, int nCode, const char* fmt, ...);

void PluginMsg(eLogLevel eLevel, const char* pszName, const char* fmt, ...);

namespace log

{
	inline void FlushLoggers() {};
}
