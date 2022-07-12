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
	extern void* (*Server_GetEntityByIndex)(int index);

	const int PERSISTENCE_MAX_SIZE = 0xD000;

	enum class ePersistenceReady : char
	{
		NOT_READY,
		READY = 3,
		READY_LOCAL = 3,
		READY_REMOTE
	};

	#pragma pack(push, 1)
	struct CBasePlayer
	{
		char pad0[0x16];

		// +0x16
		char m_Name[64];
		// +0x56

		char pad1[0x44A];

		// +0x4A0
		ePersistenceReady m_iPersistenceReady;
		// +0x4A1

		char pad2[0x59];

		// +0x4FA
		char m_PersistenceBuffer[PERSISTENCE_MAX_SIZE];

		char pad3[0x2006];

		// +0xF500
		char m_UID[32];
	};
	#pragma pack(pop)

	extern CBasePlayer*(__fastcall* UTIL_PlayerByIndex)(int playerIndex);
} // namespace R2