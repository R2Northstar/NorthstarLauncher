#include "crashhandler.h"
#include "config/profile.h"
#include "dedicated/dedicated.h"
#include "util/version.h"
#include "mods/modmanager.h"

#include <minidumpapiset.h>

#define CRASHHANDLER_MAX_FRAMES 32

//-----------------------------------------------------------------------------
// Purpose: Vectored exception callback
//-----------------------------------------------------------------------------
LONG WINAPI ExceptionFilter(EXCEPTION_POINTERS* pExceptionInfo)
{
	g_pCrashHandler->Lock();

	g_pCrashHandler->SetExceptionInfos(pExceptionInfo);

	// Don't run if we don't recognize the exception
	if (g_pCrashHandler->GetExceptionString() == g_pCrashHandler->GetExceptionString(0xFFFFFFFF))
	{
		g_pCrashHandler->Unlock();
		return EXCEPTION_CONTINUE_SEARCH;
	}

	// Don't run if a debbuger is attached
	if (IsDebuggerPresent())
	{
		g_pCrashHandler->Unlock();
		return EXCEPTION_CONTINUE_SEARCH;
	}

	// Needs to be called first as we use the members this sets later on
	g_pCrashHandler->SetCrashedModule();

	// Format
	g_pCrashHandler->FormatException();
	g_pCrashHandler->FormatCallstack();

	// Flush
	NS::log::FlushLoggers();

	// Show message box
	g_pCrashHandler->ShowPopUpMessage();

	g_pCrashHandler->Unlock();
	return EXCEPTION_EXECUTE_HANDLER;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CCrashHandler::CCrashHandler() : m_hExceptionFilter(nullptr), m_pExceptionInfos(nullptr)
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CCrashHandler::~CCrashHandler()
{
	Shutdown();
}

//-----------------------------------------------------------------------------
// Purpose: Initilazes crash handler
//-----------------------------------------------------------------------------
void CCrashHandler::Init()
{
	m_hExceptionFilter = AddVectoredExceptionHandler(TRUE, ExceptionFilter);
}

//-----------------------------------------------------------------------------
// Purpose: Shutdowns crash handler
//-----------------------------------------------------------------------------
void CCrashHandler::Shutdown()
{
	if (m_hExceptionFilter)
	{
		RemoveVectoredExceptionHandler(m_hExceptionFilter);
		m_hExceptionFilter = nullptr;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the exception info
//-----------------------------------------------------------------------------
void CCrashHandler::SetExceptionInfos(EXCEPTION_POINTERS* pExceptionPointers)
{
	m_pExceptionInfos = pExceptionPointers;
}
//-----------------------------------------------------------------------------
// Purpose: Sets the exception stirngs for message box
//-----------------------------------------------------------------------------
void CCrashHandler::SetCrashedModule()
{
	LPCSTR pCrashAddress = static_cast<LPCSTR>(m_pExceptionInfos->ExceptionRecord->ExceptionAddress);
	HMODULE hCrashedModule;
	if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, pCrashAddress, &hCrashedModule))
	{
		m_strCrashedModule = "UNKNOWN_MODULE";
		return;
	}

	// Get module filename
	CHAR szCrashedModulePath[MAX_PATH];
	GetModuleFileNameExA(GetCurrentProcess(), hCrashedModule, szCrashedModulePath, sizeof(szCrashedModulePath));

	const CHAR* pszCrashedModuleFileName = strrchr(szCrashedModulePath, '\\') + 1;

	// Get relative address
	LPCSTR pModuleBase = reinterpret_cast<LPCSTR>(pCrashAddress - reinterpret_cast<LPCSTR>(hCrashedModule));

	m_strCrashedModule = pszCrashedModuleFileName;
	m_strCrashedOffset = fmt::format("{:#x}", reinterpret_cast<DWORD64>(pModuleBase));
}

//-----------------------------------------------------------------------------
// Purpose: Gets the exception null terminated stirng
//-----------------------------------------------------------------------------

const CHAR* CCrashHandler::GetExceptionString() const
{
	return GetExceptionString(m_pExceptionInfos->ExceptionRecord->ExceptionCode);
}

//-----------------------------------------------------------------------------
// Purpose: Gets the exception null terminated stirng
//-----------------------------------------------------------------------------
const CHAR* CCrashHandler::GetExceptionString(DWORD dwExceptionCode) const
{
	// clang-format off
	switch (dwExceptionCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:         return "EXCEPTION_ACCESS_VIOLATION";
	case EXCEPTION_DATATYPE_MISALIGNMENT:    return "EXCEPTION_DATATYPE_MISALIGNMENT";
	case EXCEPTION_BREAKPOINT:               return "EXCEPTION_BREAKPOINT";
	case EXCEPTION_SINGLE_STEP:              return "EXCEPTION_SINGLE_STEP";
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
	case EXCEPTION_FLT_DENORMAL_OPERAND:     return "EXCEPTION_FLT_DENORMAL_OPERAND";
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:       return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
	case EXCEPTION_FLT_INEXACT_RESULT:       return "EXCEPTION_FLT_INEXACT_RESULT";
	case EXCEPTION_FLT_INVALID_OPERATION:    return "EXCEPTION_FLT_INVALID_OPERATION";
	case EXCEPTION_FLT_OVERFLOW:             return "EXCEPTION_FLT_OVERFLOW";
	case EXCEPTION_FLT_STACK_CHECK:          return "EXCEPTION_FLT_STACK_CHECK";
	case EXCEPTION_FLT_UNDERFLOW:            return "EXCEPTION_FLT_UNDERFLOW";
	case EXCEPTION_INT_DIVIDE_BY_ZERO:       return "EXCEPTION_INT_DIVIDE_BY_ZERO";
	case EXCEPTION_INT_OVERFLOW:             return "EXCEPTION_INT_OVERFLOW";
	case EXCEPTION_PRIV_INSTRUCTION:         return "EXCEPTION_PRIV_INSTRUCTION";
	case EXCEPTION_IN_PAGE_ERROR:            return "EXCEPTION_IN_PAGE_ERROR";
	case EXCEPTION_ILLEGAL_INSTRUCTION:      return "EXCEPTION_ILLEGAL_INSTRUCTION";
	case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
	case EXCEPTION_STACK_OVERFLOW:           return "EXCEPTION_STACK_OVERFLOW";
	case EXCEPTION_INVALID_DISPOSITION:      return "EXCEPTION_INVALID_DISPOSITION";
	case EXCEPTION_GUARD_PAGE:               return "EXCEPTION_GUARD_PAGE";
	case EXCEPTION_INVALID_HANDLE:           return "EXCEPTION_INVALID_HANDLE";
	}
	// clang-format on
	return "UNKNOWN_EXCEPTION";
}

//-----------------------------------------------------------------------------
// Purpose: Shows a message box
//-----------------------------------------------------------------------------
void CCrashHandler::ShowPopUpMessage() const
{
	if (!IsDedicatedServer())
	{
		// Create Crash Message dialog
		STARTUPINFOA si;
		PROCESS_INFORMATION pi;

		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		std::string strCmdLine =
			fmt::format("bin/CrashMsg.exe {} {} {} {}", GetNorthstarPrefix(), GetExceptionString(), m_strCrashedModule, m_strCrashedOffset);

		if (CreateProcessA(NULL, (LPSTR)strCmdLine.c_str(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
		{
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCrashHandler::FormatException()
{
	spdlog::error("-------------------------------------------");
	spdlog::error("Northstar has crashed!");
	spdlog::error("{}", GetExceptionString());
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CCrashHandler::FormatCallstack()
{
	spdlog::error("Callstack:");

	PVOID pFrames[CRASHHANDLER_MAX_FRAMES];

	int iFrames = RtlCaptureStackBackTrace(0, CRASHHANDLER_MAX_FRAMES, pFrames, NULL);

	// Whether we should skip the frame as it was called after the exception occured
	bool bSkipExceptionHandlingFrames = true;

	for (int i = 0; i < iFrames; i++)
	{
		const CHAR* pszModuleFileName;

		LPCSTR pAddress = static_cast<LPCSTR>(pFrames[i]);
		HMODULE hModule;
		if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, pAddress, &hModule))
		{
			pszModuleFileName = "UNKNOWN_MODULE";
		}
		else
		{
			CHAR szModulePath[MAX_PATH];
			GetModuleFileNameExA(GetCurrentProcess(), hModule, szModulePath, sizeof(szModulePath));
			pszModuleFileName = strrchr(szModulePath, '\\') + 1;
		}

		// Get relative address
		LPCSTR pModuleBase = reinterpret_cast<LPCSTR>(pAddress - reinterpret_cast<LPCSTR>(hModule));

		// Should we log this frame
		if (bSkipExceptionHandlingFrames)
		{
			if (m_strCrashedModule == pszModuleFileName)
			{
				bSkipExceptionHandlingFrames = false;
			}
			else
			{
				continue;
			}
		}

		// Log module + offset
		spdlog::error("\t{} + {:#x}", pszModuleFileName, reinterpret_cast<DWORD64>(pModuleBase));
	}
}

//-----------------------------------------------------------------------------
CCrashHandler* g_pCrashHandler = nullptr;
