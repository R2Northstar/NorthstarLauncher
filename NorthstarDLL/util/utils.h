#pragma once

namespace NS::Utils
{
	void RemoveAsciiControlSequences(char* str, bool allow_color_codes);
	std::string FormatV(const char* fmt, va_list vArgs);
	std::string Format(const char* fmt, ...);
} // namespace NS::Utils
