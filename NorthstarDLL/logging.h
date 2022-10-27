#pragma once
#include "pch.h"
#include "spdlog/sinks/base_sink.h"

void CreateLogFiles();
void InitialiseLogging();
void InitialiseConsole();

inline bool g_bSpdLog_UseAnsiClr = false;

// spdlog logger
class ExternalConsoleSink : public spdlog::sinks::base_sink<std::mutex>
{
  private:
	std::map<spdlog::level::level_enum, std::string> m_LogColours = {
		{spdlog::level::trace, "\033[38;2;0;255;255;49m"},
		{spdlog::level::debug, "\033[38;2;0;255;255;49m"},
		{spdlog::level::info, "\033[39;49m"},
		{spdlog::level::warn, "\033[38;2;255;255;0;49m"},
		{spdlog::level::err, "\033[38;2;255;150;150;49m"},
		{spdlog::level::critical, "\033[38;2;255;0;0;49m"},
		{spdlog::level::off, ""}};

	// this map is used to print coloured tags (strings in the form "[<tag>]") to the console
	std::map<std::string, std::string> m_contexts = {
		// UI is light blue, SV is pink, CL is light green
		{"UI SCRIPT", "\033[38;2;100;255;255;49m"},
		{"CL SCRIPT", "\033[38;2;100;255;100;49m"},
		{"SV SCRIPT", "\033[38;2;255;100;255;49m"},
		// native is just a darker version of script
		{"SV NATIVE", "\033[38;2;150;50;150;49m"},
		// echo is just a bit grey
		{"echo", "\033[38;2;150;150;150;49m"}};
	
  protected:
	void sink_it_(const spdlog::details::log_msg& msg) override;
	void flush_() override;
};
