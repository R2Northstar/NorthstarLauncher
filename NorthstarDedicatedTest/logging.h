#pragma once
#include "context.h"

// Returns number of frames found
int PrintCallStack(std::string prefix = "");

void CreateLogFiles();
void InitialiseLogging();
void InitialiseEngineSpewFuncHooks(HMODULE baseAddress);
void InitialiseClientPrintHooks(HMODULE baseAddress);