#include "pch.h"
#include "miscserverfixes.h"
#include "hookutils.h"

typedef bool (*DevTextBufferDumpToFileType)(int a1);
DevTextBufferDumpToFileType DevTextBufferDumpToFileServer;

bool DevTextBufferDumpToFileHookServer(int a1)
{ return true; }

void InitialiseMiscServerFixes(HMODULE baseAddress)
{
	// ret at the start of the concommand GenerateObjFile as it can crash servers
	{
		char* ptr = reinterpret_cast<char*>(baseAddress) + 0x38D920;
		TempReadWrite rw(ptr);
		*ptr = 0xC3;
	}

	// nop out call to VGUI shutdown since it crashes the game when quitting from the console
	{
		char* ptr = reinterpret_cast<char*>(baseAddress) + 0x154A96;
		TempReadWrite rw(ptr);
		*(ptr++) = 0x90; // nop
		*(ptr++) = 0x90; // nop
		*(ptr++) = 0x90; // nop
		*(ptr++) = 0x90; // nop
		*ptr = 0x90;	 // nop
	}

	HookEnabler hook;

	if (!strstr(GetCommandLineA(), "-allowio"))
	{
		ENABLER_CREATEHOOK(
			hook, (char*)baseAddress + 0x2A8690, &DevTextBufferDumpToFileHookServer,
			reinterpret_cast<LPVOID*>(&DevTextBufferDumpToFileServer));
	}
}