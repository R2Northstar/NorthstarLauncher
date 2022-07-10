#pragma once
#include "pch.h"
#include "dedicated.h"
#include "tier0.h"
#include "NSMem.h"

AUTOHOOK_INIT()

AUTOHOOK(D3D11CreateDevice, materialsystem_dx11.dll + 0xD9A0E,
HRESULT, __stdcall, (
	void* pAdapter,
	int DriverType,
	HMODULE Software,
	UINT Flags,
	int* pFeatureLevels,
	UINT FeatureLevels,
	UINT SDKVersion,
	void** ppDevice,
	int* pFeatureLevel,
	void** ppImmediateContext))
{
	// note: this is super duper temp pretty much just messing around with it
	// does run surprisingly well on dedi for a software driver tho if you ignore the +1gb ram usage at times, seems like dedi doesn't
	// really call gpu much even with renderthread still being a thing will be using this hook for actual d3d stubbing and stuff later

	// note: this has been succeeded by the d3d11 and gfsdk stubs, and is only being kept around for posterity and as a fallback option
	if (Tier0::CommandLine()->CheckParm("-softwared3d11"))
		DriverType = 5; // D3D_DRIVER_TYPE_WARP

	return D3D11CreateDevice(
		pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, ppDevice, pFeatureLevel, ppImmediateContext);
}

ON_DLL_LOAD_DEDI("materialsystem_dx11.dll", DedicatedServerMaterialSystem, (HMODULE baseAddress))
{
	AUTOHOOK_DISPATCH()

	// CMaterialSystem::FindMaterial
	// make the game always use the error material
	NSMem::BytePatch((uintptr_t)baseAddress + 0x5F0F1, {0xE9, 0x34, 0x03, 0x00});
}