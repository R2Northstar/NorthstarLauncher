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

	const CHAR* GetExceptionString() const;
	const CHAR* GetExceptionString(DWORD dwExceptionCode) const;

	//-----------------------------------------------------------------------------
	// Formatting
	//-----------------------------------------------------------------------------
	void ShowPopUpMessage() const;

  private:
	PVOID m_hExceptionFilter;
	EXCEPTION_POINTERS* m_pExceptionInfos;

	std::mutex m_Mutex;
};

extern CCrashHandler* g_pCrashHandler;
