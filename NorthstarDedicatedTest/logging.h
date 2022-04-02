#pragma once
#include "context.h"

void PrintCallStack(std::string prefix = "");
void CreateLogFiles();
void InitialiseLogging();
void InitialiseEngineSpewFuncHooks(HMODULE baseAddress);
void InitialiseClientPrintHooks(HMODULE baseAddress);