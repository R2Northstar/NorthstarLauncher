#pragma once

std::string FormatV(const char* pszFormat, va_list vArgs);
std::string Format(const char* pszFormat, ...);

void RemoveAsciiControlSequences(char* str, bool allow_color_codes);
