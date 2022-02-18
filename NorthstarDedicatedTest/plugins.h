#pragma once

extern GameState gameState;
extern ServerInfo serverInfo;
extern PlayerInfo playerInfo;

void initGameState();

void InitialisePluginCommands(HMODULE baseAddress);