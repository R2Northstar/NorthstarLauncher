#pragma once

// use the R2 namespace for game funcs
namespace R2
{
	enum server_state_t
	{
		ss_dead = 0, // Dead
		ss_loading, // Spawning
		ss_active, // Running
		ss_paused, // Running, but paused
	};

	extern server_state_t* g_pServerState;

	// server entity stuff
	typedef void* (*Server_GetEntityByIndexType)(int index);
	extern Server_GetEntityByIndexType Server_GetEntityByIndex;
} // namespace R2