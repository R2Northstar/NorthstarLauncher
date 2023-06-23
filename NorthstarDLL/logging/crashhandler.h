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
	void ShowPopUpMessage() const;

	void FormatException();
	void FormatCallstack();
	void FormatIntReg(const CHAR* pszRegister, DWORD64 nValue);
	void FormatFloatReg(const CHAR* pszRegister, M128A nValue);
	void FormatRegisters();
	void FormatLoadedMods();

  private:
	PVOID m_hExceptionFilter;
	EXCEPTION_POINTERS* m_pExceptionInfos;

	std::string m_strCrashedModule;
	std::string m_strCrashedOffset;

	std::mutex m_Mutex;
};

extern CCrashHandler* g_pCrashHandler;
