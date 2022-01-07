#include "pch.h"
#include "latencyflex.h"
#include "hookutils.h"
#include "dedicated.h"

typedef void(*OnRenderStartType)();
OnRenderStartType OnRenderStart;

HMODULE m_lfxModule{};
typedef void (*PFN_winelfx_WaitAndBeginFrame)();
PFN_winelfx_WaitAndBeginFrame m_winelfx_WaitAndBeginFrame{};

void OnRenderStartHook()
{
	m_winelfx_WaitAndBeginFrame();

	OnRenderStart();
}

void InitialiseLatencyFleX(HMODULE baseAddress)
{
	if (IsDedicated())
		return;

	// Connect to the LatencyFleX service
	// LatencyFleX is an open source vendor agnostic replacement for Nvidia Reflex input latency reduction technology.
	// https://ishitatsuyuki.github.io/post/latencyflex/
	m_lfxModule = LoadLibraryA("latencyflex_wine.dll");

	if (m_lfxModule == nullptr)
	{
		spdlog::info("Unable to load LatencyFleX library, LatencyFleX disabled.");
		return;
	}

	m_winelfx_WaitAndBeginFrame = reinterpret_cast<PFN_winelfx_WaitAndBeginFrame>(reinterpret_cast<void*>(GetProcAddress(m_lfxModule,"winelfx_WaitAndBeginFrame")));
	spdlog::info("LatencyFleX initialized.");

	HookEnabler hook;
	ENABLER_CREATEHOOK(hook, (char*)baseAddress + 0x1952C0, &OnRenderStartHook, reinterpret_cast<LPVOID*>(&OnRenderStart));
}