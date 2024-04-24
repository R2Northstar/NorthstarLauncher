#pragma once

inline void* C_WeaponX_vftable = nullptr;

#pragma pack(push, 1)
struct C_WeaponX
{
public:
	void* vftable;
	const char gap_8[5880];
	// is a struct in reality. size may be wrong
	const char weaponVars[0x1000];
};
#pragma pack(pop)
