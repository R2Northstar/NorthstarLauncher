#pragma once

#include <chrono>

//-----------------------------------------------------------------------------
// Purpose: Timer class to help keep track of time
//-----------------------------------------------------------------------------
class CTimer
{
  public:
	void Start(void);
	long long GetDuration(void) const;

  private:
	std::chrono::time_point<std::chrono::steady_clock> m_Clock;
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
inline void CTimer::Start(void)
{
	m_Clock = std::chrono::steady_clock::now();
}

//-----------------------------------------------------------------------------
// Purpose: Returns time passed since Start was called in milliseconds
//-----------------------------------------------------------------------------
inline long long CTimer::GetDuration(void) const
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_Clock).count();
}
