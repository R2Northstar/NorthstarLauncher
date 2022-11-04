#include "pch.h"
#include "logging.h"
#include "convar.h"
#include "concommand.h"
#include "nsprefix.h"
#include "bitbuf.h"
#include "tier0.h"
#include "spdlog/sinks/basic_file_sink.h"

#include <iomanip>
#include <sstream>

AUTOHOOK_INIT()

ConVar* Cvar_spewlog_enable;

enum class SpewType_t
{
	SPEW_MESSAGE = 0,

	SPEW_WARNING,
	SPEW_ASSERT,
	SPEW_ERROR,
	SPEW_LOG,

	SPEW_TYPE_COUNT
};

const std::unordered_map<SpewType_t, const char*> PrintSpewTypes = {
	{SpewType_t::SPEW_MESSAGE, "SPEW_MSG"},
	{SpewType_t::SPEW_WARNING, "SPEW_WARNING"},
	{SpewType_t::SPEW_ASSERT, "SPEW_ASSERT"},
	{SpewType_t::SPEW_ERROR, "SPEW_ERROR"},
	{SpewType_t::SPEW_LOG, "SPEW_LOG"}};

// these are used to define the base text colour for these things
const std::unordered_map<SpewType_t, spdlog::level::level_enum> PrintSpewLevels = {
	{SpewType_t::SPEW_MESSAGE, spdlog::level::level_enum::info},
	{SpewType_t::SPEW_WARNING, spdlog::level::level_enum::warn},
	{SpewType_t::SPEW_ASSERT, spdlog::level::level_enum::err},
	{SpewType_t::SPEW_ERROR, spdlog::level::level_enum::err},
	{SpewType_t::SPEW_LOG, spdlog::level::level_enum::info}};

const std::unordered_map<SpewType_t, const char> PrintSpewTypes_Short = {
	{SpewType_t::SPEW_MESSAGE, 'M'},
	{SpewType_t::SPEW_WARNING, 'W'},
	{SpewType_t::SPEW_ASSERT, 'A'},
	{SpewType_t::SPEW_ERROR, 'E'},
	{SpewType_t::SPEW_LOG, 'L'}};

// clang-format off
AUTOHOOK(EngineSpewFunc, engine.dll + 0x11CA80,
void, __fastcall, (void* pEngineServer, SpewType_t type, const char* format, va_list args))
// clang-format on
{
	if (!Cvar_spewlog_enable->GetBool())
		return;

	const char* typeStr = PrintSpewTypes.at(type);
	char formatted[2048] = {0};
	bool bShouldFormat = true;

	// because titanfall 2 is quite possibly the worst thing to yet exist, it sometimes gives invalid specifiers which will crash
	// ttf2sdk had a way to prevent them from crashing but it doesnt work in debug builds
	// so we use this instead
	for (int i = 0; format[i]; i++)
	{
		if (format[i] == '%')
		{
			switch (format[i + 1])
			{
			// this is fucking awful lol
			case 'd':
			case 'i':
			case 'u':
			case 'x':
			case 'X':
			case 'f':
			case 'F':
			case 'g':
			case 'G':
			case 'a':
			case 'A':
			case 'c':
			case 's':
			case 'p':
			case 'n':
			case '%':
			case '-':
			case '+':
			case ' ':
			case '#':
			case '*':
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				break;

			default:
			{
				bShouldFormat = false;
				break;
			}
			}
		}
	}

	if (bShouldFormat)
		vsnprintf(formatted, sizeof(formatted), format, args);
	else
		spdlog::warn("Failed to format {} \"{}\"", typeStr, format);

	auto endpos = strlen(formatted);
	if (formatted[endpos - 1] == '\n')
		formatted[endpos - 1] = '\0'; // cut off repeated newline

	spdlog::log(PrintSpewLevels.at(type), "[SV NATIVE] [{}] {}", PrintSpewTypes_Short.at(type), formatted);
}

// used for printing the output of status
// clang-format off
AUTOHOOK(Status_ConMsg, engine.dll + 0x15ABD0,
void,, (const char* text, ...))
// clang-format on
{
	char formatted[2048];
	va_list list;

	va_start(list, text);
	vsprintf_s(formatted, text, list);
	va_end(list);

	auto endpos = strlen(formatted);
	if (formatted[endpos - 1] == '\n')
		formatted[endpos - 1] = '\0'; // cut off repeated newline

	spdlog::info(formatted);
}

// clang-format off
AUTOHOOK(CClientState_ProcessPrint, engine.dll + 0x1A1530, 
bool,, (void* thisptr, uintptr_t msg))
// clang-format on
{
	char* text = *(char**)(msg + 0x20);

	auto endpos = strlen(text);
	if (text[endpos - 1] == '\n')
		text[endpos - 1] = '\0'; // cut off repeated newline

	spdlog::info(text);
	return true;
}

ConVar* Cvar_cl_showtextmsg;

class ICenterPrint
{
  public:
	virtual void ctor() = 0;
	virtual void Clear(void) = 0;
	virtual void ColorPrint(int r, int g, int b, int a, wchar_t* text) = 0;
	virtual void ColorPrint(int r, int g, int b, int a, char* text) = 0;
	virtual void Print(wchar_t* text) = 0;
	virtual void Print(char* text) = 0;
	virtual void SetTextColor(int r, int g, int b, int a) = 0;
};

ICenterPrint* pInternalCenterPrint = NULL;

enum class TextMsgPrintType_t
{
	HUD_PRINTNOTIFY = 1,
	HUD_PRINTCONSOLE,
	HUD_PRINTTALK,
	HUD_PRINTCENTER
};

// clang-format off
AUTOHOOK(TextMsg, client.dll + 0x198710,
void,, (BFRead* msg))
// clang-format on
{
	TextMsgPrintType_t msg_dest = (TextMsgPrintType_t)msg->ReadByte();

	char text[256];
	msg->ReadString(text, sizeof(text));

	if (!Cvar_cl_showtextmsg->GetBool())
		return;

	switch (msg_dest)
	{
	case TextMsgPrintType_t::HUD_PRINTCENTER:
		pInternalCenterPrint->Print(text);
		break;

	default:
		spdlog::warn("Unimplemented TextMsg type {}! printing to console", msg_dest);
		[[fallthrough]];

	case TextMsgPrintType_t::HUD_PRINTCONSOLE:
		auto endpos = strlen(text);
		if (text[endpos - 1] == '\n')
			text[endpos - 1] = '\0'; // cut off repeated newline

		spdlog::info(text);
		break;
	}
}

// clang-format off
AUTOHOOK(ConCommand_echo, engine.dll + 0x123680,
void,, (const CCommand& arg))
// clang-format on
{
	if (arg.ArgC() >= 2)
		spdlog::info("[echo] {}", arg.ArgS());
}

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
			sink->set_pattern("[%H:%M:%S] [%l] %v");
			spdlog::default_logger()->sinks().push_back(sink);
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
	spdlog::memory_buf_t formatted;
	spdlog::sinks::base_sink<std::mutex>::formatter_->format(msg, formatted);

	std::string out = "";
	// if ansi colour is turned off, just use std::cout and return
	if (!g_bSpdLog_UseAnsiClr)
	{
		out += fmt::to_string(formatted);
		goto WRITE_CONSOLE;
	}

	// get the message "tags" (bits of string surrounded with [])
	// try to get the colour for each "tag"
	// print to the console with colours
	{
		// get message string
		std::string str = fmt::to_string(formatted);
		// should probably be a stack in case of nested tags but honestly i do not care
		std::string baseCol = m_LogColours[msg.level];
		out = baseCol; // always start by setting the colour
		for (int i = 0; i < str.length(); ++i)
		{
			char c = str[i];
			// add the char to the output
			out += c;
			// we are only looking for [ for bonus stuff
			if (c != '[')
				continue;

			// this could probably be done better with substr and find?
			// breaks when someone does something like: "thing [ stuff [UI SCRIPT]" which shouldn't rly happen in practice tbh
			std::string buf = "";
			while (++i < str.length() && str[i] != ']')
			{
				buf += str[i];
			}

			// if its an unknown tag (no colour), then just use the current colour
			if (m_tags.find(buf) != m_tags.end())
				out += m_tags[buf];

			// add the buf
			out += buf;
			// reset back to baseCol
			out += baseCol;
			// add the ] so it doesn't get missed
			out += str[i];
		}
		// end the string with an ansi reset just for safety
		out += "\033[39;39m";
	}
	WRITE_CONSOLE:
	// print the string to the console - this is definitely bad i think
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	auto ignored = WriteConsoleA(handle, out.c_str(), std::strlen(out.c_str()), nullptr, nullptr);
	(void)ignored;
}

void ExternalConsoleSink::flush_()
{
	std::cout << std::flush;
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

	if (!strstr(GetCommandLineA(), "-noansiclr"))
	{
		DWORD dwMode = NULL;
		HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);

		GetConsoleMode(hOutput, &dwMode);
		dwMode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;

		if (!SetConsoleMode(hOutput, dwMode)) // Some editions of Windows have 'VirtualTerminalLevel' disabled by default.
		{
			// Warn the user if 'VirtualTerminalLevel' could not be set on users environment.
			MessageBoxA(
				NULL,
				"Failed to set console mode 'VirtualTerminalLevel'.\n"
				"Please add the '-noansiclr' parameter and restart \nthe game if output logging appears distorted.",
				"Northstar Warning",
				MB_ICONWARNING | MB_OK);
		}
	}
}

void InitialiseLogging()
{
	// create a logger, and set it to default
	spdlog::set_default_logger(std::make_shared<spdlog::logger>("logger"));

	// set whether we should use colour in the logs
	g_bSpdLog_UseAnsiClr = !strstr(GetCommandLineA(), "-noansiclr");

	// create our console sink
	auto sink = std::make_shared<ExternalConsoleSink>();
	// set the pattern
	if (g_bSpdLog_UseAnsiClr)
		sink->set_pattern("[%H:%M:%S] %^%v%$"); // dont put the log level in the pattern if we are using colours, as the colour will show the log level
	else
		sink->set_pattern("[%H:%M:%S] [%l] %v");


	// add our sink to the logger
	spdlog::default_logger()->sinks().push_back(sink);
}

ON_DLL_LOAD_RELIESON("engine.dll", EngineSpewFuncHooks, ConVar, (CModule module))
{
	AUTOHOOK_DISPATCH_MODULE(engine.dll)

	Cvar_spewlog_enable = new ConVar("spewlog_enable", "1", FCVAR_NONE, "Enables/disables whether the engine spewfunc should be logged");
}

ON_DLL_LOAD_CLIENT_RELIESON("client.dll", ClientPrintHooks, ConVar, (CModule module))
{
	AUTOHOOK_DISPATCH_MODULE(client.dll)

	Cvar_cl_showtextmsg = new ConVar("cl_showtextmsg", "1", FCVAR_NONE, "Enable/disable text messages printing on the screen.");
	pInternalCenterPrint = module.Offset(0x216E940).As<ICenterPrint*>();
}
