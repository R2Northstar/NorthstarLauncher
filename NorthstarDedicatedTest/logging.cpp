#include "pch.h"
#include "logging.h"
#include "sourceconsole.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "hookutils.h"
#include "dedicated.h"
#include "convar.h"
#include "configurables.h"
#include <iomanip>
#include <sstream>
#include <Psapi.h>
#include <minidumpapiset.h>
#include <unordered_set>
#include <fstream>

std::string GetAdvancedDebugInfo(CONTEXT* threadContext)
{
	std::stringstream output;

	output << "// This file contains information for advanced debugging of stack-corrupting Northstar crashes.\n";
	output << "// This Northstar.dll was build on " __DATE__ "\n";
#ifdef _DEBUG
	output << "// We were running in DEBUG mode\n";
#else
	output << "// We were running in RELEASE mode\n";
#endif
	MEMORYSTATUSEX memInfo;
	memInfo.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&memInfo);
	output << "\n\nMEMORY STATUS:";
	output << "\tSwap + Ram Usage: " << memInfo.ullTotalPageFile << std::endl;
	output << "\tPhysical Usage: " << memInfo.ullTotalPhys << std::endl;
	output << "\tVirtul Usage: " << memInfo.ullTotalPhys << std::endl;
	output << "\tAvailable: " << memInfo.ullAvailVirtual << std::endl;

	uintptr_t stackMax = threadContext->Rsp - sizeof(void*) + 1;
	uintptr_t stackMin = stackMax - 0x10000;

	struct ModuleMemoryInfo
	{
		uintptr_t baseAddress;
		uintptr_t textSectionStart, textSectionSize;
	};

	std::map<std::string, ModuleMemoryInfo> moduleInfos;

	// Loop through all of our modules
	HMODULE hMods[1024];
	DWORD cbNeeded;
	if (EnumProcessModules(GetCurrentProcess(), hMods, sizeof(hMods), &cbNeeded))
	{
		for (int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
		{
			char szModName[MAX_PATH];
			if (GetModuleFileNameA(hMods[i], szModName, MAX_PATH)) // Check if valid module at all
			{
				uintptr_t addr = (uintptr_t)hMods[i];
				auto dosHeader = (IMAGE_DOS_HEADER*)addr;
				auto ntHeaders = (IMAGE_NT_HEADERS*)(addr + dosHeader->e_lfanew);
				if (ntHeaders->Signature == IMAGE_NT_SIGNATURE) // Make sure its a real module
				{
					auto firstSection = IMAGE_FIRST_SECTION(ntHeaders);
					if (!strcmp((const char*)firstSection->Name, ".text")) // Make sure this is actually the text section
					{
						moduleInfos[strrchr(szModName, '\\') + 1] =
							ModuleMemoryInfo {addr, addr + firstSection->VirtualAddress, firstSection->SizeOfRawData};
					}
				}
			}
		}
	}

	output << "MODULES LOADED:\n";
	for (auto moduleEntry : moduleInfos)
	{
		output << "\t" << (void*)moduleEntry.second.baseAddress << ": " << moduleEntry.first << std::endl;
	}

	std::stringstream stackModuleDump, stackRawDataDump;
	stackRawDataDump << std::hex << "\t";

	for (uintptr_t i = stackMax; i >= stackMin; i--)
	{
		if (IsBadReadPtr((void*)i, sizeof(void*)))
			break;

		stackRawDataDump << std::setw(2) << std::setfill('0') << (int)(*(BYTE*)i);
		if (i < stackMax && i % 32 == 0)
			stackRawDataDump << "\n\t";

		auto curStackPtr = *(uintptr_t*)i;
		if (!curStackPtr)
			continue;

		for (auto moduleEntry : moduleInfos)
		{
			auto& modInfo = moduleEntry.second;
			if (curStackPtr >= modInfo.textSectionStart && curStackPtr < modInfo.textSectionStart + modInfo.textSectionSize)
			{
				stackModuleDump << "\t{" << (void*)i << "} " << (void*)curStackPtr << " [" << moduleEntry.first << " + "
								<< (void*)(curStackPtr - modInfo.baseAddress) << "]\n";
				break;
			}
		}
	}

	output << "STACK POTENTIAL FRAMES:\n" << stackModuleDump.str() << std::endl;
	output << "FULL STACK DUMP:\n" << stackRawDataDump.str() << std::endl;

	return output.str();
}

int PrintCallStack(std::string prefix)
{
	PVOID framesToCapture[62];
	int frames = RtlCaptureStackBackTrace(0, 62, framesToCapture, NULL);
	for (int i = 0; i < frames; i++)
	{
		HMODULE backtraceModuleHandle;
		GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, static_cast<LPCSTR>(framesToCapture[i]), &backtraceModuleHandle);

		char backtraceModuleFullName[MAX_PATH];
		GetModuleFileNameExA(GetCurrentProcess(), backtraceModuleHandle, backtraceModuleFullName, MAX_PATH);
		char* backtraceModuleName = strrchr(backtraceModuleFullName, '\\') + 1;

		void* actualAddress = (void*)framesToCapture[i];
		void* relativeAddress = (void*)(uintptr_t(actualAddress) - uintptr_t(backtraceModuleHandle));

		spdlog::error(prefix + "{} + {} ({})", backtraceModuleName, relativeAddress, actualAddress);
	}
	return frames;
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
		// todo: might be good to delete logs that are too old
		time_t time = std::time(nullptr);
		tm currentTime = *std::localtime(&time);
		std::stringstream stream;

		stream << std::put_time(&currentTime, (GetNorthstarPrefix() + "/logs/nslog%Y-%m-%d %H-%M-%S.txt").c_str());
		spdlog::default_logger()->sinks().push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(stream.str(), false));
		spdlog::flush_on(spdlog::level::info);
	}
}

long __stdcall ExceptionFilter(EXCEPTION_POINTERS* exceptionInfo)
{
	static bool logged = false;
	if (logged)
		return EXCEPTION_CONTINUE_SEARCH;

	if (!IsDebuggerPresent())
	{
		EXCEPTION_RECORD* record = exceptionInfo->ExceptionRecord;

		// Ideally we could just ignore uncaught instead... but there seem to be so many not listed (i.e. E06D7363, etc.)
		const static std::unordered_set<DWORD> caughtExceptionCodes = {
			EXCEPTION_ACCESS_VIOLATION,
			EXCEPTION_DATATYPE_MISALIGNMENT,
			EXCEPTION_ARRAY_BOUNDS_EXCEEDED,
			EXCEPTION_FLT_DENORMAL_OPERAND,
			EXCEPTION_FLT_DIVIDE_BY_ZERO,
			EXCEPTION_FLT_INEXACT_RESULT,
			EXCEPTION_FLT_INVALID_OPERATION,
			EXCEPTION_FLT_OVERFLOW,
			EXCEPTION_FLT_STACK_CHECK,
			EXCEPTION_FLT_UNDERFLOW,
			EXCEPTION_INT_DIVIDE_BY_ZERO,
			EXCEPTION_INT_OVERFLOW,
			EXCEPTION_PRIV_INSTRUCTION,
			EXCEPTION_IN_PAGE_ERROR,
			EXCEPTION_ILLEGAL_INSTRUCTION,
			EXCEPTION_NONCONTINUABLE_EXCEPTION,
			EXCEPTION_STACK_OVERFLOW,
			EXCEPTION_GUARD_PAGE,
		};

		if (caughtExceptionCodes.find(record->ExceptionCode) == caughtExceptionCodes.end())
			return EXCEPTION_CONTINUE_SEARCH; // didnt ask + dont care

		std::stringstream exceptionCause;
		exceptionCause << "Crash Exception Cause: ";

		switch (record->ExceptionCode)
		{
		case EXCEPTION_ACCESS_VIOLATION:
		case EXCEPTION_IN_PAGE_ERROR:
		{

			exceptionCause << "Access Violation (";

			auto exceptionInfoType = record->ExceptionInformation[0];
			auto exceptionInfo1 = (void*)record->ExceptionInformation[1];

			switch (exceptionInfoType)
			{
			case 0:
				exceptionCause << "Attempted to read from: 0x" << exceptionInfo1;
				break;
			case 1:
				exceptionCause << "Attempted to write to: 0x" << exceptionInfo1;
				break;
			case 8:
				exceptionCause << "Data Execution Prevention (DEP) at: 0x" << exceptionInfo1;
				break;
			default:
				exceptionCause << "Unknown access violation at: 0x" << exceptionInfo1;
			}

			exceptionCause << ")";
			break;
		}
		default:
		{

			static HANDLE ntdll = GetModuleHandleA("ntdll.dll");
			char buf[MAX_PATH] = {0};
			DWORD res = FormatMessageA(
				FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE,
				ntdll,
				record->ExceptionCode,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				buf,
				MAX_PATH,
				NULL);

			exceptionCause << buf;
			break;
		}
		}

		exceptionCause << " [" << (void*)record->ExceptionCode << "]";

		std::stringstream crashDevInfo;

		void* exceptionAddress = record->ExceptionAddress;

		HMODULE crashedModuleHandle;
		GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, static_cast<LPCSTR>(exceptionAddress), &crashedModuleHandle);

		MODULEINFO crashedModuleInfo;
		GetModuleInformation(GetCurrentProcess(), crashedModuleHandle, &crashedModuleInfo, sizeof(crashedModuleInfo));

		char crashedModuleFullName[MAX_PATH];
		GetModuleFileNameExA(GetCurrentProcess(), crashedModuleHandle, crashedModuleFullName, MAX_PATH);
		char* crashedModuleName = strrchr(crashedModuleFullName, '\\') + 1;

		DWORD64 crashedModuleOffset = ((DWORD64)exceptionAddress) - ((DWORD64)crashedModuleInfo.lpBaseOfDll);
		CONTEXT* exceptionContext = exceptionInfo->ContextRecord;

		printf_s("\n\n");
		spdlog::error("###### NORTHSTAR HAS CRASHED :( ######");
		spdlog::error("A minidump has been written and exception information for developers/staff is available below:\n");
		spdlog::error(exceptionCause.str());
		spdlog::error("Crashing Instruction Address: {} + {}", crashedModuleName, (void*)crashedModuleOffset);

		int callstackSize = PrintCallStack("\t");

		{ // Print all registers using minimal code (a little silly, but it works well and is easier on the eyes)
			std::stringstream regDataLog;
			regDataLog << std::hex;

#define P(regName) << #regName << "=" << exceptionContext->regName << ", "
			regDataLog P(Rax) P(Rbx) P(Rcx) P(Rdx) P(Rdi) P(Rbp) P(Rsp) P(R8) P(R9) P(R10) P(R11) P(R12) P(R13) P(R14) P(R15);
#undef P
			spdlog::error("Registers: " + regDataLog.str());
		}

		constexpr const char* quotes[] = {
			"Protocol three: I will NOT lose another pilot."
			"We had no other option.",
			"It is good to see you too, Pilot.",
			"Noted.",
			"Look, a developer! Now the odds are in our favor!",
			"It was fun.",
			"And sometimes, when you fall, you fly.",
			"Go get em, tiger.",
			"Support ticket sighted, moving to engage.",
		};

		time_t time = std::time(nullptr);
		tm currentTime = *std::localtime(&time);

		std::stringstream dumpPathStream;
		dumpPathStream << std::put_time(&currentTime, (GetNorthstarPrefix() + "/logs/nsdump %Y-%m-%d %H-%M-%S.dmp").c_str());
		auto hMinidumpFile =
			CreateFileA(dumpPathStream.str().c_str(), GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		if (hMinidumpFile)
		{
			MINIDUMP_EXCEPTION_INFORMATION dumpExceptionInfo;
			dumpExceptionInfo.ThreadId = GetCurrentThreadId();
			dumpExceptionInfo.ExceptionPointers = exceptionInfo;
			dumpExceptionInfo.ClientPointers = false;

			MiniDumpWriteDump(
				GetCurrentProcess(),
				GetCurrentProcessId(),
				hMinidumpFile,
				MINIDUMP_TYPE(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory),
				&dumpExceptionInfo,
				nullptr,
				nullptr);
			CloseHandle(hMinidumpFile);
		}
		else
		{
			spdlog::error("Failed to write minidump file to {}!", dumpPathStream.str());
		}

		std::stringstream advDbgInfoPathStream;
		advDbgInfoPathStream << std::put_time(
			&currentTime, (GetNorthstarPrefix() + "/logs/ns-advanced-debug-info %Y-%m-%d %H-%M-%S.ndi").c_str());

		// Write advanced debug info
		std::ofstream advDbgInfoDump = std::ofstream(advDbgInfoPathStream.str());
		advDbgInfoDump << GetAdvancedDebugInfo(exceptionContext);
		advDbgInfoDump.close();
		spdlog::error(
			"Wrote advanced debugging information to \"{}\".\n\n\t\"{}\"", advDbgInfoPathStream.str(), quotes[time % ARRAYSIZE(quotes)]);

		if (!IsDedicated())
			MessageBoxA(
				0, "Northstar has crashed! Crash info can be found in R2Northstar/logs.", "Northstar has crashed!", MB_ICONERROR | MB_OK);
	}

	logged = true;
	return EXCEPTION_EXECUTE_HANDLER;
}

HANDLE hExceptionFilter;

BOOL WINAPI ConsoleHandlerRoutine(DWORD eventCode)
{
	switch (eventCode)
	{
	case CTRL_CLOSE_EVENT:
		// User closed console, shut everything down
		spdlog::info("Exiting due to console close...");
		RemoveVectoredExceptionHandler(hExceptionFilter);
		exit(EXIT_SUCCESS);
		return FALSE;
	}

	return TRUE;
}

void InitialiseLogging()
{
	hExceptionFilter = AddVectoredExceptionHandler(TRUE, ExceptionFilter);

	SetConsoleCtrlHandler(ConsoleHandlerRoutine, true);
}

ConVar* Cvar_spewlog_enable;

enum SpewType_t
{
	SPEW_MESSAGE = 0,

	SPEW_WARNING,
	SPEW_ASSERT,
	SPEW_ERROR,
	SPEW_LOG,

	SPEW_TYPE_COUNT
};

typedef void (*EngineSpewFuncType)();
EngineSpewFuncType EngineSpewFunc;

void EngineSpewFuncHook(void* engineServer, SpewType_t type, const char* format, va_list args)
{
	if (!Cvar_spewlog_enable->GetBool())
		return;

	const char* typeStr;
	switch (type)
	{
	case SPEW_MESSAGE:
	{
		typeStr = "SPEW_MESSAGE";
		break;
	}

	case SPEW_WARNING:
	{
		typeStr = "SPEW_WARNING";
		break;
	}

	case SPEW_ASSERT:
	{
		typeStr = "SPEW_ASSERT";
		break;
	}

	case SPEW_ERROR:
	{
		typeStr = "SPEW_ERROR";
		break;
	}

	case SPEW_LOG:
	{
		typeStr = "SPEW_LOG";
		break;
	}

	default:
	{
		typeStr = "SPEW_UNKNOWN";
		break;
	}
	}

	char formatted[2048] = {0};
	bool shouldFormat = true;

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
				shouldFormat = false;
				break;
			}
			}
		}
	}

	if (shouldFormat)
		vsnprintf(formatted, sizeof(formatted), format, args);
	else
	{
		spdlog::warn("Failed to format {} \"{}\"", typeStr, format);
	}

	auto endpos = strlen(formatted);
	if (formatted[endpos - 1] == '\n')
		formatted[endpos - 1] = '\0'; // cut off repeated newline

	spdlog::info("[SERVER {}] {}", typeStr, formatted);
}

typedef void (*Status_ConMsg_Type)(const char* text, ...);
Status_ConMsg_Type Status_ConMsg_Original;

void Status_ConMsg_Hook(const char* text, ...)
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

typedef bool (*CClientState_ProcessPrint_Type)(__int64 thisptr, __int64 msg);
CClientState_ProcessPrint_Type CClientState_ProcessPrint_Original;

bool CClientState_ProcessPrint_Hook(__int64 thisptr, __int64 msg)
{
	char* text = *(char**)(msg + 0x20);

	auto endpos = strlen(text);
	if (text[endpos - 1] == '\n')
		text[endpos - 1] = '\0'; // cut off repeated newline

	spdlog::info(text);
	return true;
}

void InitialiseEngineSpewFuncHooks(HMODULE baseAddress)
{
	HookEnabler hook;

	ENABLER_CREATEHOOK(hook, (char*)baseAddress + 0x11CA80, EngineSpewFuncHook, reinterpret_cast<LPVOID*>(&EngineSpewFunc));

	// Hook print function that status concmd uses to actually print data
	ENABLER_CREATEHOOK(hook, (char*)baseAddress + 0x15ABD0, Status_ConMsg_Hook, reinterpret_cast<LPVOID*>(&Status_ConMsg_Original));

	// Hook CClientState::ProcessPrint
	ENABLER_CREATEHOOK(
		hook,
		(char*)baseAddress + 0x1A1530,
		CClientState_ProcessPrint_Hook,
		reinterpret_cast<LPVOID*>(&CClientState_ProcessPrint_Original));

	Cvar_spewlog_enable = new ConVar("spewlog_enable", "1", FCVAR_NONE, "Enables/disables whether the engine spewfunc should be logged");
}

#include "bitbuf.h"

ConVar* Cvar_cl_showtextmsg;

typedef void (*TextMsg_Type)(__int64);
TextMsg_Type TextMsg_Original;

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

ICenterPrint* internalCenterPrint = NULL;

void TextMsgHook(BFRead* msg)
{
	int msg_dest = msg->ReadByte();

	char text[256];
	msg->ReadString(text, sizeof(text));

	if (!Cvar_cl_showtextmsg->GetBool())
		return;

	switch (msg_dest)
	{
	case 4: // HUD_PRINTCENTER
		internalCenterPrint->Print(text);
		break;
	default:
		spdlog::warn("Unimplemented TextMsg type {}! printing to console", msg_dest);
		[[fallthrough]];
	case 2: // HUD_PRINTCONSOLE
		auto endpos = strlen(text);
		if (text[endpos - 1] == '\n')
			text[endpos - 1] = '\0'; // cut off repeated newline
		spdlog::info(text);
		break;
	}
}

void InitialiseClientPrintHooks(HMODULE baseAddress)
{
	HookEnabler hook;

	internalCenterPrint = (ICenterPrint*)((char*)baseAddress + 0x216E940);

	// "TextMsg" usermessage
	ENABLER_CREATEHOOK(hook, (char*)baseAddress + 0x198710, TextMsgHook, reinterpret_cast<LPVOID*>(&TextMsg_Original));

	Cvar_cl_showtextmsg = new ConVar("cl_showtextmsg", "1", FCVAR_NONE, "Enable/disable text messages printing on the screen.");
}