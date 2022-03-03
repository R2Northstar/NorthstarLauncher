#pragma once

extern ConVar* CVar_rcon_address;
extern ConVar* CVar_rcon_password;
extern ConVar* CVar_sv_rcon_debug;
extern ConVar* CVar_sv_rcon_maxfailures;
extern ConVar* CVar_sv_rcon_maxignores;
extern ConVar* CVar_sv_rcon_maxsockets;
extern ConVar* CVar_sv_rcon_sendlogs;
extern ConVar* CVar_sv_rcon_whitelist_address;

void InitializeRconSystems(HMODULE baseAddress);
