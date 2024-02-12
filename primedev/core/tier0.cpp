#include "tier0.h"

AUTOHOOK_INIT()

IMemAlloc* g_pMemAllocSingleton = nullptr;

CommandLineType CommandLine;
Plat_FloatTimeType Plat_FloatTime;
ThreadInServerFrameThreadType ThreadInServerFrameThread;

typedef IMemAlloc* (*CreateGlobalMemAllocType)();
CreateGlobalMemAllocType CreateGlobalMemAlloc;

// needs to be a seperate function, since memalloc.cpp calls it
void TryCreateGlobalMemAlloc()
{
	// init memalloc stuff
	CreateGlobalMemAlloc =
		reinterpret_cast<CreateGlobalMemAllocType>(GetProcAddress(GetModuleHandleA("tier0.dll"), "CreateGlobalMemAlloc"));
	g_pMemAllocSingleton = CreateGlobalMemAlloc(); // if it already exists, this returns the preexisting IMemAlloc instance
}

AUTOHOOK_PROCADDRESS(ThreadSetDebugName, tier0.dll, ThreadSetDebugName, void, __fastcall, (HANDLE threadHandle, const char* name))
{
	if (threadHandle == 0)
		threadHandle = GetCurrentThread();

	// need to grab it dynamically as this function was only introduced at some point in Windows 10
	static decltype(&SetThreadDescription) _SetThreadDescription =
		CModule("KernelBase.dll").GetExportedFunction("SetThreadDescription").RCast<decltype(&SetThreadDescription)>();

	// TODO: This "method" of "charset conversion" from string to wstring is abhorrent. Change it to a proper one
	// as soon as Northstar has some helper function to do proper charset conversions.
	auto tmp = std::string(name);
	if (_SetThreadDescription)
		_SetThreadDescription(threadHandle, std::wstring(tmp.begin(), tmp.end()).c_str());

	ThreadSetDebugName(threadHandle, name);
}

ON_DLL_LOAD("tier0.dll", Tier0GameFuncs, (CModule module))
{
	// shouldn't be necessary, but do this just in case
	TryCreateGlobalMemAlloc();

	AUTOHOOK_DISPATCH()

	// setup tier0 funcs
	CommandLine = module.GetExportedFunction("CommandLine").RCast<CommandLineType>();
	Plat_FloatTime = module.GetExportedFunction("Plat_FloatTime").RCast<Plat_FloatTimeType>();
	ThreadInServerFrameThread = module.GetExportedFunction("ThreadInServerFrameThread").RCast<ThreadInServerFrameThreadType>();
}
