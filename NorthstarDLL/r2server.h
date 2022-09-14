#pragma once

// use the R2 namespace for game funcs
namespace R2
{
	// server entity stuff
	class CBaseEntity;
	extern CBaseEntity* (*Server_GetEntityByIndex)(int index);

#pragma pack(push, 1)
	struct CBasePlayer
	{
		char pad[88];
		int m_nPlayerIndex;
	};
#pragma pack(pop)

	extern CBasePlayer*(__fastcall* UTIL_PlayerByIndex)(int playerIndex);
} // namespace R2
