#include "pch.h"
#include "convar.h"
#include "NSMem.h"

ON_DLL_LOAD_CLIENT_RELIESON("client.dll", MiscClientFixes, ConVar, (HMODULE baseAddress)) {
}