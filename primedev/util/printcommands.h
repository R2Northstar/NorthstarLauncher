#pragma once
#include "core/tier1/cmd.h"

void PrintCommandHelpDialogue(const ConCommandBase* command, const char* name);
void TryPrintCvarHelpForCommand(const char* pCommand);
void InitialiseCommandPrint();
