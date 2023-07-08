#include "logging.h"
#include "core/convar/convar.h"
#include "core/convar/concommand.h"
#include "config/profile.h"
#include "core/tier0.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_sinks.h"

#include <iomanip>
#include <sstream>
#include <shellapi.h>

//-----------------------------------------------------------------------------
// Purpose: Checks if install folder is writable, exits if it is not
//-----------------------------------------------------------------------------
void SpdLog_PreInit(void)
{
	// This is called before SpdLog_Init so we can't use any logging helpers

	// NOTE [Fifty]: Instead of checking every reason as to why we might not be able to write to a directory
	//               it is easier to just try to create a file
	FILE* pFile = fopen(".nstemp", "w");
	if (pFile)
	{
		fclose(pFile);
		remove(".nstemp");
		return;
	}

	// Show message box
	int iAction = MessageBoxA(
		NULL,
		"The current directory isn't writable!\n"
		"Please move the game into a writable directory to be able to continue\n\n"
		"Click \"OK\" to open the wiki in your browser",
		"Northstar Error",
		MB_ICONERROR | MB_OKCANCEL);

	// User chose to open the troubleshooting wiki page
	if (iAction == IDOK)
	{
		ShellExecuteA(
			NULL,
			NULL,
			"https://r2northstar.gitbook.io/r2northstar-wiki/installing-northstar/"
			"troubleshooting#cannot-write-log-file-when-using-northstar-on-ea-app",
			NULL,
			NULL,
			SW_NORMAL);
	}

	// Gracefully close
	TerminateProcess(GetCurrentProcess(), -1);
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void SpdLog_Init(void) {
	g_WinLogger = spdlog::stdout_logger_mt("win_console");
	spdlog::set_default_logger(g_WinLogger);
	g_WinLogger->set_level(spdlog::level::trace);

	g_WinLogger->set_pattern("[0.000] %v");
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void SpdLog_Shutdown(void) {}
//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void Console_Init(void)
{
	bool bShow = strstr(GetCommandLineA(), "-wconsole") != NULL;

	if (bShow)
	{
		(void)AllocConsole();

		FILE* pDummy;
		freopen_s(&pDummy, "CONIN$", "r", stdin);
		freopen_s(&pDummy, "CONOUT$", "w", stdout);
		freopen_s(&pDummy, "CONOUT$", "w", stderr);
	}
	else
	{
		// TODO [Fifty]: Only close console once gamewindow is created
		HWND hConsole = GetConsoleWindow();
		(void)FreeConsole();
		(void)PostMessageA(hConsole, WM_CLOSE, 0, 0);
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void Console_Shutdown(void) {}

AUTOHOOK_INIT()

std::vector<std::shared_ptr<ColoredLogger>> loggers {};

namespace NS::log
{
	std::shared_ptr<ColoredLogger> SCRIPT_UI;
	std::shared_ptr<ColoredLogger> SCRIPT_CL;
	std::shared_ptr<ColoredLogger> SCRIPT_SV;

	std::shared_ptr<ColoredLogger> NATIVE_UI;
	std::shared_ptr<ColoredLogger> NATIVE_CL;
	std::shared_ptr<ColoredLogger> NATIVE_SV;
	std::shared_ptr<ColoredLogger> NATIVE_EN;

	std::shared_ptr<ColoredLogger> fs;
	std::shared_ptr<ColoredLogger> rpak;
	std::shared_ptr<ColoredLogger> echo;

	std::shared_ptr<ColoredLogger> NORTHSTAR;
	std::shared_ptr<ColoredLogger> PLUGINSYS;
}; // namespace NS::log

// This needs to be called after hooks are loaded so we can access the command line args
void CreateLogFiles()
{
	if (strstr(GetCommandLineA(), "-disablelogs"))
	{
		spdlog::default_logger()->set_level(spdlog::level::off);
	}
	else
	{
		try
		{
			// todo: might be good to delete logs that are too old
			time_t time = std::time(nullptr);
			tm currentTime = *std::localtime(&time);
			std::stringstream stream;

			stream << std::put_time(&currentTime, (GetNorthstarPrefix() + "/logs/nslog%Y-%m-%d %H-%M-%S.txt").c_str());
			auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(stream.str(), false);
			sink->set_pattern("[%H:%M:%S] [%n] [%l] %v");
			for (auto& logger : loggers)
			{
				logger->sinks().push_back(sink);
			}
			spdlog::flush_on(spdlog::level::info);
		}
		catch (...)
		{
			spdlog::error("Failed creating log file!");
			MessageBoxA(
				0, "Failed creating log file! Make sure the profile directory is writable.", "Northstar Warning", MB_ICONWARNING | MB_OK);
		}
	}
}

void ExternalConsoleSink::sink_it_(const spdlog::details::log_msg& msg)
{
	throw std::runtime_error("sink_it_ called on SourceConsoleSink with pure log_msg. This is an error!");
}

void ExternalConsoleSink::custom_sink_it_(const custom_log_msg& msg)
{
	spdlog::memory_buf_t formatted;
	spdlog::sinks::base_sink<std::mutex>::formatter_->format(msg, formatted);

	std::string out = "";
	// if ansi colour is turned off, just use WriteConsoleA and return
	if (!g_bSpdLog_UseAnsiColor)
	{
		out += fmt::to_string(formatted);
	}

	// print to the console with colours
	else
	{
		// get message string
		std::string str = fmt::to_string(formatted);

		std::string levelColor = m_LogColours[msg.level];
		std::string name {msg.logger_name.begin(), msg.logger_name.end()};

		std::string name_str = "[NAME]";
		int name_pos = str.find(name_str);
		str.replace(name_pos, name_str.length(), msg.origin->ANSIColor + "[" + name + "]" + default_color);

		std::string level_str = "[LVL]";
		int level_pos = str.find(level_str);
		str.replace(level_pos, level_str.length(), levelColor + "[" + std::string(level_names[msg.level]) + "]" + default_color);

		out += str;
	}
	// print the string to the console - this is definitely bad i think
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	auto ignored = WriteConsoleA(handle, out.c_str(), std::strlen(out.c_str()), nullptr, nullptr);
	(void)ignored;
}

void ExternalConsoleSink::flush_()
{
	std::cout << std::flush;
}

void CustomSink::custom_log(const custom_log_msg& msg)
{
	std::lock_guard<std::mutex> lock(mutex_);
	custom_sink_it_(msg);
}

void InitialiseConsole()
{
	if (AllocConsole() == FALSE)
	{
		std::cout << "[*] Failed to create a console window, maybe a console already exists?" << std::endl;
	}
	else
	{
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
	}

	// this if statement is adapted from r5sdk
	if (!strstr(GetCommandLineA(), "-noansiclr"))
	{
		g_bSpdLog_UseAnsiColor = true;
		DWORD dwMode = 0;
		HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);

		GetConsoleMode(hOutput, &dwMode);
		dwMode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;

		if (!SetConsoleMode(hOutput, dwMode)) // Some editions of Windows have 'VirtualTerminalLevel' disabled by default.
		{
			// If 'VirtualTerminalLevel' can't be set, just disable ANSI color, since it wouldnt work anyway.
			spdlog::warn("could not set VirtualTerminalLevel. Disabling color output");
			g_bSpdLog_UseAnsiColor = false;
		}
	}
}

void RegisterLogger(std::shared_ptr<ColoredLogger> logger)
{
	loggers.push_back(logger);
}

void RegisterCustomSink(std::shared_ptr<CustomSink> sink)
{
	for (auto& logger : loggers)
	{
		logger->custom_sinks_.push_back(sink);
	}
};

void InitialiseLogging()
{
	// create a logger, and set it to default
	NS::log::NORTHSTAR = std::make_shared<ColoredLogger>("NORTHSTAR", NS::Colors::NORTHSTAR, true);
	NS::log::NORTHSTAR->sinks().clear();
	loggers.push_back(NS::log::NORTHSTAR);
	spdlog::set_default_logger(NS::log::NORTHSTAR);

	// create our console sink
	auto sink = std::make_shared<ExternalConsoleSink>();
	// set the pattern
	if (g_bSpdLog_UseAnsiColor)
		// dont put the log level in the pattern if we are using colours, as the colour will show the log level
		sink->set_pattern("[%H:%M:%S] [NAME] [LVL] %v");
	else
		sink->set_pattern("[%H:%M:%S] [%n] [%l] %v");

	// add our sink to the logger
	NS::log::NORTHSTAR->custom_sinks_.push_back(sink);

	NS::log::SCRIPT_UI = std::make_shared<ColoredLogger>("SCRIPT UI", NS::Colors::SCRIPT_UI);
	NS::log::SCRIPT_CL = std::make_shared<ColoredLogger>("SCRIPT CL", NS::Colors::SCRIPT_CL);
	NS::log::SCRIPT_SV = std::make_shared<ColoredLogger>("SCRIPT SV", NS::Colors::SCRIPT_SV);

	NS::log::NATIVE_UI = std::make_shared<ColoredLogger>("NATIVE UI", NS::Colors::NATIVE_UI);
	NS::log::NATIVE_CL = std::make_shared<ColoredLogger>("NATIVE CL", NS::Colors::NATIVE_CL);
	NS::log::NATIVE_SV = std::make_shared<ColoredLogger>("NATIVE SV", NS::Colors::NATIVE_SV);
	NS::log::NATIVE_EN = std::make_shared<ColoredLogger>("NATIVE EN", NS::Colors::NATIVE_ENGINE);

	NS::log::fs = std::make_shared<ColoredLogger>("FILESYSTM", NS::Colors::FILESYSTEM);
	NS::log::rpak = std::make_shared<ColoredLogger>("RPAK_FSYS", NS::Colors::RPAK);
	NS::log::echo = std::make_shared<ColoredLogger>("ECHO", NS::Colors::ECHO);

	NS::log::PLUGINSYS = std::make_shared<ColoredLogger>("PLUGINSYS", NS::Colors::PLUGINSYS);

	loggers.push_back(NS::log::SCRIPT_UI);
	loggers.push_back(NS::log::SCRIPT_CL);
	loggers.push_back(NS::log::SCRIPT_SV);

	loggers.push_back(NS::log::NATIVE_UI);
	loggers.push_back(NS::log::NATIVE_CL);
	loggers.push_back(NS::log::NATIVE_SV);
	loggers.push_back(NS::log::NATIVE_EN);

	loggers.push_back(NS::log::PLUGINSYS);

	loggers.push_back(NS::log::fs);
	loggers.push_back(NS::log::rpak);
	loggers.push_back(NS::log::echo);
}
