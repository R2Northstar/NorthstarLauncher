#pragma once

void RemoveAsciiControlSequences(char* str, bool allow_color_codes);

std::string FormatV(const char* fmt, va_list vArgs);
std::string Format(const char* fmt, ...);

std::string CreateTimeStamp();

class ScopeGuard
{
public:
	auto operator=(ScopeGuard&) = delete;
	ScopeGuard(ScopeGuard&) = delete;

	ScopeGuard(std::function<void()> callback) : m_callback(callback) {}
	~ScopeGuard()
	{
		m_callback();
	}
	void Dismiss()
	{
		m_callback = [] {};
	}

private:
	std::function<void()> m_callback;
};
