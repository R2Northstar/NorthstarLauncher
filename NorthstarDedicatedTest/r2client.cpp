#include "pch.h"
#include "r2client.h"

using namespace R2;

// use the R2 namespace for game funcs
namespace R2
{
	char* g_LocalPlayerUserID;
	char* g_LocalPlayerOriginToken;
	GetBaseLocalClientType GetBaseLocalClient;
} // namespace R2

ON_DLL_LOAD("engine.dll", R2EngineClient, [](HMODULE baseAddress)
{
	g_LocalPlayerUserID = (char*)baseAddress + 0x13F8E688;
	g_LocalPlayerOriginToken = (char*)baseAddress + 0x13979C80;

	GetBaseLocalClient = (GetBaseLocalClientType)((char*)baseAddress + 0x78200);
})