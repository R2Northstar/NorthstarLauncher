#include "pch.h"
#include "tier0.h"

// use the Tier0 namespace for tier0 funcs
namespace Tier0
{
	IMemAlloc* g_pMemAllocSingleton;

	ErrorType Error;
	CommandLineType CommandLine;
	Plat_FloatTimeType Plat_FloatTime;
	ThreadInServerFrameThreadType ThreadInServerFrameThread;
} // namespace Tier0

typedef Tier0::IMemAlloc* (*CreateGlobalMemAllocType)();
CreateGlobalMemAllocType CreateGlobalMemAlloc;

// needs to be a seperate function, since memalloc.cpp calls it
void TryCreateGlobalMemAlloc()
{
	// init memalloc stuff
	CreateGlobalMemAlloc = reinterpret_cast<CreateGlobalMemAllocType>(GetProcAddress(GetModuleHandleA("tier0.dll"), "CreateGlobalMemAlloc"));
	Tier0::g_pMemAllocSingleton = CreateGlobalMemAlloc(); // if it already exists, this returns the preexisting IMemAlloc instance
}

ON_DLL_LOAD("tier0.dll", Tier0GameFuncs, (HMODULE baseAddress)
{
	// shouldn't be necessary, but do this just in case
	TryCreateGlobalMemAlloc();

	// setup tier0 funcs
	Tier0::Error = reinterpret_cast<Tier0::ErrorType>(GetProcAddress(baseAddress, "Error"));
	Tier0::CommandLine = reinterpret_cast<Tier0::CommandLineType>(GetProcAddress(baseAddress, "CommandLine"));
	Tier0::Plat_FloatTime = reinterpret_cast<Tier0::Plat_FloatTimeType>(GetProcAddress(baseAddress, "Plat_FloatTime"));
	Tier0::ThreadInServerFrameThread =
		reinterpret_cast<Tier0::ThreadInServerFrameThreadType>(GetProcAddress(baseAddress, "ThreadInServerFrameThread"));
})