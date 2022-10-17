#pragma once

void CreateLogFiles();
void InitialiseLogging();
void InitialiseConsole();
void InitialiseEngineSpewFuncHooks(HMODULE baseAddress);
void InitialiseClientPrintHooks(HMODULE baseAddress);
