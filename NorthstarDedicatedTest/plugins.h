#pragma once
#include "plugin_abi.h"

int GetServerInfoChar(char* out_buf, size_t out_buf_len, ServerInfoType var);
int GetServerInfoInt(int* out_ptr, ServerInfoType var);
int GetServerInfoBool(bool* out_ptr, ServerInfoType var);

int GetGameStateChar(char* out_buf, size_t out_buf_len, GameStateInfoType var);
int GetGameStateInt(int* out_ptr, GameStateInfoType var);
int GetGameStateBool(bool* out_ptr, GameStateInfoType var);

int GetPlayerInfoChar(char* out_buf, size_t out_buf_len, PlayerInfoType var);
int GetPlayerInfoInt(int* out_ptr, PlayerInfoType var);
int GetPlayerInfoBool(bool* out_ptr, PlayerInfoType var);

void InitGameState();
void* GetPluginObject(PluginObject var);

void InitialisePluginCommands(HMODULE baseAddress);