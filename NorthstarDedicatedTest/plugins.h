#pragma once
#include "plugin_abi.h"
extern InternalGameState gameState;
extern InternalServerInfo serverInfo;
extern InternalPlayerInfo playerInfo;

void initGameState();

void InitialisePluginCommands(HMODULE baseAddress);