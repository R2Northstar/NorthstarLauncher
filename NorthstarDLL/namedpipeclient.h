#pragma once
#include "pch.h"
#include "convar.h"

extern ConVar* Cvar_ns_server_name;

//bool shouldUseNamedPipe = true;
void InitialiseNamedPipeClient(HMODULE baseAddress);
//void InitialiseNamedPipeClient();
