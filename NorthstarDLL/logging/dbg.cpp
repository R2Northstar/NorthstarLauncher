#include "dbg.h"

#include "util/utils.h"
#include "logging/sourceconsole.h"
#include "logging/logging.h"
#include <dedicated/dedicatedlogtoclient.h>
#include <regex>
#include "dedicated/dedicated.h"

const std::regex AnsiRegex("\\\033\\[.*?m");
std::mutex g_LogMutex;

//-----------------------------------------------------------------------------
// Purpose: Get the log context string
// Input  : eContext -
// Output : Zero terminated string
//-----------------------------------------------------------------------------
const char* Log_GetContextString(eLog eContext)
{
	return sLogString[static_cast<int>(eContext)];
}

//-----------------------------------------------------------------------------
// Purpose: Get the color for a given context and level combination
// Input  : eContext -
//          eLevel -
// Output : The color
//-----------------------------------------------------------------------------
Color Log_GetColor(eLog eContext, eLogLevel eLevel)
{
	if (eLevel == eLogLevel::LOG_WARN)
		return Color(200, 200, 50);
	if (eLevel == eLogLevel::LOG_ERROR)
		return Color(220, 50, 50);

	switch (eContext)
	{
	case eLog::NS:
		return Color(50, 110, 170);
	case eLog::SCRIPT_SERVER:
		return Color(120, 110, 50);
	case eLog::SCRIPT_CLIENT:
		return Color(90, 110, 40);
	case eLog::SCRIPT_UI:
		return Color(70, 110, 90);
	case eLog::SERVER:
		return Color(200, 160, 80);
	case eLog::CLIENT:
		return Color(170, 200, 80);
	case eLog::UI:
		return Color(110, 200, 150);
	case eLog::ENGINE:
		return Color(110, 180, 150);
	case eLog::RTECH:
		return Color(220, 160, 40);
	case eLog::FS:
		return Color(40, 220, 210);
	case eLog::MAT:
		return Color(40, 230, 120);
	case eLog::AUDIO:
		return Color(130, 130, 220);
	case eLog::VIDEO:
		return Color(200, 130, 200);
	case eLog::MS:
		return Color(40, 110, 220);
	case eLog::MODSYS:
		return Color(20, 100, 150);
	case eLog::PLUGSYS:
		return Color(190, 190, 90);
	case eLog::PLUGIN:
		return Color(190, 140, 90);
	case eLog::CHAT:
		return Color(220, 180, 70);
	}
	return Color(255, 255, 255);
}

//-----------------------------------------------------------------------------
// Purpose: Get logger based on the log level
// Input  : eLevel -
// Output : Smart pointer to the logger
//-----------------------------------------------------------------------------
std::shared_ptr<spdlog::logger> Log_GetLogger(eLogLevel eLevel)
{
	std::string svName;

	switch (eLevel)
	{
	case eLogLevel::LOG_INFO:
		svName = "northstar(info)";
		break;
	case eLogLevel::LOG_WARN:
		svName = "northstar(warning)";
		break;
	case eLogLevel::LOG_ERROR:
		svName = "northstar(error)";
		break;
	}

	return spdlog::get(svName);
}

//-----------------------------------------------------------------------------
// Purpose: Prints to all outputs based on parameters, va_list version
// Input  : eContext -
//          eLevel -
//          iCode -
//          *pszName -
//          *fmt -
//          vArgs -
//-----------------------------------------------------------------------------
void CoreMsgV(eLog eContext, eLogLevel eLevel, const int iCode, const char* pszName, const char* fmt, va_list vArgs)
{
	std::string svMessage;

	//-----------------------------------
	// Format header
	if (g_bConsole_UseAnsiColor)
	{
		std::string svAnsiString = Log_GetColor(eContext, eLevel).ToANSIColor();
		svMessage += Format("%s[%s] ", svAnsiString.c_str(), pszName);
	}
	else
	{
		svMessage += Format("[%s] ", pszName);
	}

	// Add the message itself
	svMessage += FormatV(fmt, vArgs);

	//-----------------------------------
	// Emit to all loggers
	//-----------------------------------
	std::lock_guard<std::mutex> lock(g_LogMutex);

	g_WinLogger->debug("{}", svMessage);

	// Remove ansi escape sequences
	svMessage = std::regex_replace(svMessage, AnsiRegex, "");

	// Log to file
	std::shared_ptr<spdlog::logger> pLogger = Log_GetLogger(eLevel);
	if (pLogger.get()) // "-nologfiles" or programmer error can cause this to fail
		pLogger->info("{:s}", svMessage);

	// Log to clients if enabled
	DediClientMsg(svMessage.c_str());

	// Log to game console
	// TODO [Fifty]: Use "VEngineCvar007" interface to print instead of this fuckery
	if (g_bEngineVguiInitilazed && (*g_pSourceGameConsole)->m_pConsole)
	{
		(*g_pSourceGameConsole)->m_pConsole->m_pConsolePanel->ColorPrint(Log_GetColor(eContext, eLevel).ToSourceColor(), svMessage.c_str());
		//(*g_pSourceGameConsole)->m_pConsole->m_pConsolePanel->Print(svMessage.c_str());
	}

	//-----------------------------------
	// Terminate process if needed
	//-----------------------------------
	if (iCode)
	{
		if (!IsDedicatedServer())
			MessageBoxA(NULL, svMessage.c_str(), "Northstar Error", MB_ICONERROR | MB_OK);

		TerminateProcess(GetCurrentProcess(), iCode);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Prints to all outputs based on parameters
// Input  : eContext -
//          eLevel -
//          iCode -
//          *pszName -
//          *fmt -
//          ... -
//-----------------------------------------------------------------------------
void CoreMsg(eLog eContext, eLogLevel eLevel, const int iCode, const char* pszName, const char* fmt, ...)
{
	va_list vArgs;
	va_start(vArgs, fmt);
	CoreMsgV(eContext, eLevel, iCode, pszName, fmt, vArgs);
	va_end(vArgs);
}

//-----------------------------------------------------------------------------
// Purpose: Prints a LOG_INFO level message
// Input  : eContext -
//          *fmt -
//          ... -
//-----------------------------------------------------------------------------
void DevMsg(eLog eContext, const char* fmt, ...)
{
	const char* pszContext = Log_GetContextString(eContext);

	va_list vArgs;
	va_start(vArgs, fmt);
	CoreMsgV(eContext, eLogLevel::LOG_INFO, 0, pszContext, fmt, vArgs);
	va_end(vArgs);
}

//-----------------------------------------------------------------------------
// Purpose: Prints a LOG_WARNING level message
// Input  : eContext -
//          *fmt -
//          ... -
//-----------------------------------------------------------------------------
void Warning(eLog eContext, const char* fmt, ...)
{
	const char* pszContext = Log_GetContextString(eContext);

	va_list vArgs;
	va_start(vArgs, fmt);
	CoreMsgV(eContext, eLogLevel::LOG_WARN, 0, pszContext, fmt, vArgs);
	va_end(vArgs);
}

//-----------------------------------------------------------------------------
// Purpose: Prints a LOG_ERROR level message
// Input  : eContext -
//          nCode - The code to terminate with, 0 means we don't teminate
//          *fmt -
//          ... -
//-----------------------------------------------------------------------------
void Error(eLog eContext, int nCode, const char* fmt, ...)
{
	const char* pszContext = Log_GetContextString(eContext);

	va_list vArgs;
	va_start(vArgs, fmt);
	CoreMsgV(eContext, eLogLevel::LOG_ERROR, nCode, pszContext, fmt, vArgs);
	va_end(vArgs);
}

//-----------------------------------------------------------------------------
// Purpose: Prints a message with a custom header, ONLY USE FOR PLUGINS OR RENAME
// Input  : eLevel -
//          *pszName -
//          *fmt -
//          ... -
//-----------------------------------------------------------------------------
void PluginMsg(eLogLevel eLevel, const char* pszName, const char* fmt, ...)
{
	va_list vArgs;
	va_start(vArgs, fmt);
	CoreMsgV(eLog::PLUGIN, eLevel, 0, pszName, fmt, vArgs);
	va_end(vArgs);
}
