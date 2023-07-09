#include "dbg.h"

#include "util/utils.h"
#include "logging/sourceconsole.h"
#include "logging/logging.h"
#include <dedicated/dedicatedlogtoclient.h>
#include <regex>

const std::regex AnsiRegex("\\\033\\[.*?m");

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char* Log_GetContextString(eLog eContext)
{
	return sLogString[static_cast<int>(eContext)];
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
Color Log_GetColor(eLog eContext, eLogLevel eLevel)
{
	/*
	Color SCRIPT_UI		(100, 255, 255);
	Color SCRIPT_CL		(100, 255, 100);
	Color SCRIPT_SV		(255, 100, 255);
	Color NATIVE_UI		(50 , 150, 150);
	Color NATIVE_CL		(50 , 150, 50 );
	Color NATIVE_SV		(150, 50 , 150);
	Color NATIVE_ENGINE	(252, 133, 153);
	Color FILESYSTEM	(0  , 150, 150);
	Color RPAK			(255, 190, 0  );
	Color NORTHSTAR		(66 , 72 , 128);
	Color ECHO			(150, 150, 159);
	Color PLUGINSYS		(244, 60 , 14);
	Color PLUGIN		(244, 106, 14);
	*/
	if (eLevel == eLogLevel::LOG_WARN)
		return Color(255,255,0);
	if (eLevel == eLogLevel::LOG_ERROR)
		return Color(255,50,50);

	switch (eContext)
	{
	case eLog::NS:
		return Color(66,72,128);
	case eLog::SCRIPT_SERVER:
		return Color(90, 80, 50);
	case eLog::SCRIPT_CLIENT:
		return Color(60, 90, 40);
	case eLog::SCRIPT_UI:
		return Color(40, 80, 90);
	case eLog::SERVER:
		return Color(170, 130, 80);
	case eLog::CLIENT:
		return Color(140, 170, 80);
	case eLog::UI:
		return Color(80, 170, 150);
	case eLog::ENGINE:
		return Color(80, 90, 180);
	case eLog::RTECH:
		return Color(220, 160, 40);
	case eLog::FS:
		return Color(40, 220, 210);
	case eLog::MAT:
		return Color(40, 230, 120);
	case eLog::AUDIO:
		return Color(40, 40, 220);
	case eLog::VIDEO:
		return Color(220, 40, 170);
	case eLog::MS:
		return Color(40, 110, 220);
	case eLog::MODSYS:
		return Color(20, 100, 150);
	case eLog::PLUGSYS:
		return Color(140, 110, 20);
	case eLog::CHAT:
		return Color(220, 180, 70);
	}
	return Color(255, 255, 255);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CoreMsgV(eLog eContext, eLogLevel eLevel, const int iCode, const char* pszName, const char* fmt, va_list vArgs)
{
	std::string svMessage;

	//-----------------------------------
	// Format header
	//-----------------------------------
	if (g_bSpdLog_UseAnsiColor)
	{
		std::string pszAnsiString = Log_GetColor(eContext, eLevel).ToANSIColor();
		svMessage += NS::Utils::Format("%s[%s] ", pszAnsiString.c_str(), pszName);
	}
	else
	{
		svMessage += NS::Utils::Format("[%s] ", pszName);
	}

	// Add the message itself
	svMessage += NS::Utils::FormatV(fmt, vArgs);

	// Log to windows console
	g_WinLogger->debug("{}", svMessage);

	// Remove ansi escape sequences
	svMessage = std::regex_replace(svMessage, AnsiRegex, "");

	// Log to client if enabled
	DediClientMsg(svMessage.c_str());

	// TODO [Fifty]: Use "VEngineCvar007" interface to print instead of this fuckery
	if (g_bEngineVguiInitilazed && (*g_pSourceGameConsole)->m_pConsole)
	{
		(*g_pSourceGameConsole)->m_pConsole->m_pConsolePanel->ColorPrint(Log_GetColor(eContext, eLevel).ToSourceColor(), svMessage.c_str());
		//(*g_pSourceGameConsole)->m_pConsole->m_pConsolePanel->Print(svMessage.c_str());
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CoreMsg(eLog eContext, eLogLevel eLevel, const int iCode, const char* pszName, const char* fmt, ...)
{
	va_list vArgs;
	va_start(vArgs, fmt);
	CoreMsgV(eContext, eLevel, iCode, pszName, fmt, vArgs);
	va_end(vArgs);
}

//-----------------------------------------------------------------------------
// Purpose:
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
// Purpose:
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
// Purpose:
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
// Purpose:
//-----------------------------------------------------------------------------
void PluginMsg(eLogLevel eLevel, const char* pszName, const char* fmt, ...)
{
	va_list vArgs;
	va_start(vArgs, fmt);
	CoreMsgV(eLog::NONE, eLevel, 0, pszName, fmt, vArgs);
	va_end(vArgs);
}
