#include "pch.h"

#include "NSMem.h"

ON_DLL_LOAD("server.dll", MiscServerFixes, (HMODULE baseAddress))
{
	uintptr_t ba = (uintptr_t)baseAddress;

	// nop out call to VGUI shutdown since it crashes the game when quitting from the console
	NSMem::NOP(ba + 0x154A96, 5);
}