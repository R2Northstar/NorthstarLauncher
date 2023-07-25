#pragma once

#include <mutex>

//-----------------------------------------------------------------------------
// Purpose: Exception handling
//-----------------------------------------------------------------------------
class CCrashHandler
{
  public:
	CCrashHandler();
	~CCrashHandler();

	void Init();
	void Shutdown();

	void Lock()
	{
		m_Mutex.lock();
	}

	void Unlock()
	{
		m_Mutex.unlock();
	}

	//-----------------------------------------------------------------------------
	// Exception helpers
	//-----------------------------------------------------------------------------
	void SetExceptionInfos(EXCEPTION_POINTERS* pExceptionPointers);

	void SetCrashedModule();

	const CHAR* GetExceptionString() const;
	const CHAR* GetExceptionString(DWORD dwExceptionCode) const;

	//-----------------------------------------------------------------------------
	// Formatting
	//-----------------------------------------------------------------------------
	void ShowPopUpMessage();

	void FormatException();
	void FormatCallstack();
	void FormatFlags(const CHAR* pszRegister, DWORD nValue);
	void FormatIntReg(const CHAR* pszRegister, DWORD64 nValue);
	void FormatFloatReg(const CHAR* pszRegister, M128A nValue);
	void FormatRegisters();
	void FormatLoadedMods();
	void FormatLoadedPlugins();
	void FormatModules();

	//-----------------------------------------------------------------------------
	// Minidump
	//-----------------------------------------------------------------------------
	void WriteMinidump();

  private:
	PVOID m_hExceptionFilter;
	EXCEPTION_POINTERS* m_pExceptionInfos;

	bool m_bHasSetConsolehandler;

	bool m_bHasShownCrashMsg;

	std::string m_svCrashedModule;
	std::string m_svCrashedOffset;

	std::string m_svError;

	std::mutex m_Mutex;
};

extern CCrashHandler* g_pCrashHandler;
