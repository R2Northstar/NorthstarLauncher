#include "dbg.h"

#include "util/utils.h"
#include "logging/sourceconsole.h"
#include "logging/logging.h"

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CoreMsgV(eLog eContext, eLogLevel eLevel, const int iCode, const char* pszName, const char* fmt, va_list vArgs)
{
	std::string svMessage;
	svMessage += NS::Utils::Format("[%s] ", pszName);
	svMessage += NS::Utils::FormatV(fmt, vArgs);

	g_WinLogger->debug("{}", svMessage);

	// TODO [Fifty]: Use "VEngineCvar007" interface to print instead of this fuckery
	if (g_bEngineVguiInitilazed && (*g_pSourceGameConsole)->m_pConsole)
	{
		(*g_pSourceGameConsole)->m_pConsole->m_pConsolePanel->Print(svMessage.c_str());
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
	va_list vArgs;
	va_start(vArgs, fmt);
	CoreMsgV(eContext, eLogLevel::LOG_INFO, 0, sLogString[(int)eContext], fmt, vArgs);
	va_end(vArgs);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void Warning(eLog eContext, const char* fmt, ...)
{
	va_list vArgs;
	va_start(vArgs, fmt);
	CoreMsgV(eContext, eLogLevel::LOG_WARN, 0, sLogString[(int)eContext], fmt, vArgs);
	va_end(vArgs);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void Error(eLog eContext, int nCode, const char* fmt, ...)
{
	va_list vArgs;
	va_start(vArgs, fmt);
	CoreMsgV(eContext, eLogLevel::LOG_ERROR, nCode, sLogString[(int)eContext], fmt, vArgs);
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
