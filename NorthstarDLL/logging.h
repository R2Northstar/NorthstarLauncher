#pragma once

void CreateLogFiles();
void InitialiseLogging();
void InitialiseEngineSpewFuncHooks(HMODULE baseAddress);
void InitialiseClientPrintHooks(HMODULE baseAddress);
