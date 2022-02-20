#include "pch.h"
#include "concommand.h"
#include "gameutils.h"
#include "misccommands.h"
#include <iostream>

typedef void (*ConCommandConstructorType)(
	ConCommand* newCommand, const char* name, void (*callback)(const CCommand&), const char* helpString, int flags, void* parent);
ConCommandConstructorType conCommandConstructor;

void RegisterConCommand(const char* name, void (*callback)(const CCommand&), const char* helpString, int flags)
{
	spdlog::info("Registering ConCommand {}", name);

	// no need to free this ever really, it should exist as long as game does
	ConCommand* newCommand = new ConCommand;
	conCommandConstructor(newCommand, name, callback, helpString, flags, nullptr);
}

void InitialiseConCommands(HMODULE baseAddress)
{
	conCommandConstructor = (ConCommandConstructorType)((char*)baseAddress + 0x415F60);
	AddMiscConCommands();
}

//-----------------------------------------------------------------------------
// Purpose: Add's flags to ConCommand.
// Input  : nFlags -
//-----------------------------------------------------------------------------
void ConCommandBase::AddFlags(int nFlags) { m_nFlags |= nFlags; }

//-----------------------------------------------------------------------------
// Purpose: Removes flags from ConCommand.
// Input  : nFlags -
//-----------------------------------------------------------------------------
void ConCommandBase::RemoveFlags(int nFlags) { m_nFlags &= ~nFlags; }

//-----------------------------------------------------------------------------
// Purpose: Checks if ConCommand has requested flags.
// Input  : nFlags -
// Output : True if ConCommand has nFlags.
//-----------------------------------------------------------------------------
bool ConCommandBase::HasFlags(int nFlags) { return m_nFlags & nFlags; }
