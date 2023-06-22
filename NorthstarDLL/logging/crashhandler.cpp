#include "crashhandler.h"
#include "config/profile.h"
#include "dedicated/dedicated.h"
#include "util/version.h"
#include "mods/modmanager.h"

#include <minidumpapiset.h>

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

	// Format

	// Flush

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
		STARTUPINFOA si {0};
		PROCESS_INFORMATION pi {0};

		std::string strCommandLine = "bin/CrashMsg.exe";
		strCommandLine += " " + GetNorthstarPrefix();
		strCommandLine += " " + std::string(GetExceptionString());

		if (CreateProcessA(NULL, (LPSTR)strCommandLine.c_str(), NULL, FALSE, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
		{
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
	}
}

//-----------------------------------------------------------------------------
CCrashHandler* g_pCrashHandler = nullptr;
