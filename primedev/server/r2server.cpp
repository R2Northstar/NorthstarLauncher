#include "r2server.h"

CBaseEntity* (*Server_GetEntityByIndex)(int index);
CBasePlayer*(__fastcall* UTIL_PlayerByIndex)(int playerIndex);

uint64_t* GlobalEntList = nullptr;
const char* (*GetWeaponName)(int index);

static bool (__fastcall* o_pOnWeaponAttack)(uint64_t a1, int a2) = nullptr;
static bool __fastcall h_pOnWeaponAttack(uint64_t a1, int a2)
{
	int owner_index = *(int32_t*)(a1 + 0xEB8);
	void* player = (void*)GlobalEntList[6 * (unsigned __int16)owner_index + 1];
	int weapon_index = *(int32_t*)(a1 + 0x12D8);
	auto weapon_name = GetWeaponName(weapon_index);
	int shotsFired = 1;
	auto player_inst = g_pSquirrel[ScriptContext::SERVER]->__sq_createscriptinstance((void*)player);
	auto weapon_inst = g_pSquirrel[ScriptContext::SERVER]->__sq_createscriptinstance((void*)a1);
	g_pSquirrel[ScriptContext::SERVER]->Call("CodeCallback_OnWeaponAttack", player_inst, weapon_inst, weapon_name, shotsFired);
	return o_pOnWeaponAttack(a1, a2);
}

ON_DLL_LOAD("server.dll", R2GameServer, (CModule module))
{
	Server_GetEntityByIndex = module.Offset(0xFB820).RCast<CBaseEntity* (*)(int)>();
	UTIL_PlayerByIndex = module.Offset(0x26AA10).RCast<CBasePlayer*(__fastcall*)(int)>();
	GetWeaponName = module.Offset(0x691300).RCast<const char* (*)(int)>();
	GlobalEntList = *module.Offset(0xB6AB58).RCast<uint64_t**>();

	o_pOnWeaponAttack = module.Offset(0x6A0220).RCast<decltype(o_pOnWeaponAttack)>();
	HookAttach(&(PVOID&)o_pOnWeaponAttack, (PVOID)h_pOnWeaponAttack);
}
