#pragma once
#include "client/weaponx.h"

#pragma pack(push, 1)
struct CWeaponX
{
	void* vftable;
	const char gap_8[5128];
	// this is used by the game as the initial offset
	// for getting eWeaponVar values. No idea about
	// the actual datatype. Start of another struct?
	const char weaponVars[0x1000];
};
#pragma pack(pop)
