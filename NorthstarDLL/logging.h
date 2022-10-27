#pragma once
#include "pch.h"
#include "spdlog/sinks/base_sink.h"

void CreateLogFiles();
void InitialiseLogging();
void InitialiseConsole();

inline bool g_bSpdLog_UseAnsiClr = false;

// spdlog logger, for cool colour things
class ExternalConsoleSink : public spdlog::sinks::base_sink<std::mutex>
{
  private:
	std::map<spdlog::level::level_enum, std::string> m_LogColours = {
		{spdlog::level::trace, "\033[38;2;0;255;255;49m"},
		{spdlog::level::debug, "\033[38;2;0;255;255;49m"},
		{spdlog::level::info, "\033[39;49m"}, // this could maybe be just white but idk the nice white that console uses
		{spdlog::level::warn, "\033[38;2;255;255;0;49m"},
		{spdlog::level::err, "\033[38;2;255;100;100;49m"},
		{spdlog::level::critical, "\033[38;2;255;0;0;49m"},
		{spdlog::level::off, ""}};

	// this map is used to print coloured tags (strings in the form "[<tag>]") to the console
	// if you add stuff to this, mimic the changes in sourceconsole.h
	std::map<std::string, std::string> m_tags = {
		// UI is light blue, SV is pink, CL is light green
		{"UI SCRIPT", "\033[38;2;100;255;255;49m"},
		{"CL SCRIPT", "\033[38;2;100;255;100;49m"},
		{"SV SCRIPT", "\033[38;2;255;100;255;49m"},
		// native is just a darker version of script
		// tbh unsure if CL and UI will ever be used? 
		{"UI NATIVE", "\033[38;2;50;150;150;49m"},
		{"CL NATIVE", "\033[38;2;50;150;50;49m"},
		{"SV NATIVE", "\033[38;2;150;50;150;49m"},
		// cool launcher things (filesystem, etc.) add more as they come up
		{"FS NATIVE", "\033[38;2;255;190;0;49m"}, // orange
		// echo is just a bit grey
		{"echo", "\033[38;2;150;150;150;49m"}};
	
  protected:
	void sink_it_(const spdlog::details::log_msg& msg) override;
	void flush_() override;
};
