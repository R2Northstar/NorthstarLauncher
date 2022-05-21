#include "pch.h"
#include "hooks.h"
#include "convar.h"
#include "hookutils.h"
#include "NSMem.h"

ON_DLL_LOAD_CLIENT_RELIESON("client.dll", MiscClientFixes, ConVar, [](HMODULE baseAddress)
{
})