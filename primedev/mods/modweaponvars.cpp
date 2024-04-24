
#include "client/weaponx.h"
#include "server/weaponx.h"
#include "modweaponvars.h"
#include "squirrel/squirrel.h"
#include "client/r2client.h"
#include "server/r2server.h"

template <ScriptContext context> WeaponVarInfo* weaponVarArray;

template <ScriptContext context> bool IsWeapon(void** ent)
{
	if (context == ScriptContext::SERVER)
		return *ent == CWeaponX_vftable;
	else
		return *ent == C_WeaponX_vftable;
}

AUTOHOOK_INIT()

// TODO: do a code callback after recalculation of mod values
// to reapply script weapon vars
// this is an alternative, but potentially could be bad
// would be way less performant tho - no idea how bad sq performance is
AUTOHOOK(Cl_UpdateWeaponVars, client.dll + 0x5B2670, char, __fastcall, (void* a2))
{
	return '\0';
}

ADD_SQFUNC("void", ScriptWeaponVars_SetInt, "entity weapon, int weaponVar, int value", "", ScriptContext::SERVER | ScriptContext::CLIENT)
{
	void** ent = g_pSquirrel<context>->getentity<void*>(sqvm, 1);
	if (!IsWeapon<context>(ent))
	{
		g_pSquirrel<context>->raiseerror(sqvm, "Entity is not a weapon");
		return SQRESULT_ERROR;
	}
	int weaponVar = g_pSquirrel<context>->getinteger(sqvm, 2);
	int value = g_pSquirrel<context>->getinteger(sqvm, 3);
	if (weaponVar < 1 || weaponVar > WEAPON_VAR_COUNT) // weapon vars start at one and end at 725 inclusive
	{
		// invalid weapon var index
		g_pSquirrel<context>->raiseerror(sqvm, "Invalid eWeaponVar!");
		return SQRESULT_ERROR;
	}

	WeaponVarInfo* varInfo = &weaponVarArray<context>[weaponVar];
	if (varInfo->type != WeaponVarType::INTEGER)
	{
		// invalid type used
		g_pSquirrel<context>->raiseerror(sqvm, "weaponVar type is not integer!");
		return SQRESULT_ERROR;
	}

	if (context == ScriptContext::SERVER)
	{
		CWeaponX* weapon = (CWeaponX*)ent;
		*(int*)(&weapon->weaponVars[varInfo->offset]) = value;
	}
	else // if (context == ScriptContext::CLIENT)
	{
		C_WeaponX* weapon = (C_WeaponX*)ent;
		*(int*)(&weapon->weaponVars[varInfo->offset]) = value;
	}

	return SQRESULT_NULL;
}

ADD_SQFUNC(
	"void", ScriptWeaponVars_SetFloat, "entity weapon, int weaponVar, float value", "", ScriptContext::SERVER | ScriptContext::CLIENT)
{
	void** ent = g_pSquirrel<context>->getentity<void*>(sqvm, 1);
	if (!IsWeapon<context>((void**)ent))
	{
		g_pSquirrel<context>->raiseerror(sqvm, "Entity is not a weapon");
		return SQRESULT_ERROR;
	}
	int weaponVar = g_pSquirrel<context>->getinteger(sqvm, 2);
	float value = g_pSquirrel<context>->getfloat(sqvm, 3);
	if (weaponVar < 1 || weaponVar > WEAPON_VAR_COUNT) // weapon vars start at one and end at 725 inclusive
	{
		// invalid weapon var index
		g_pSquirrel<context>->raiseerror(sqvm, "Invalid eWeaponVar!");
		return SQRESULT_ERROR;
	}

	WeaponVarInfo* varInfo = &weaponVarArray<context>[weaponVar];
	if (varInfo->type != WeaponVarType::FLOAT32)
	{
		// invalid type used
		g_pSquirrel<context>->raiseerror(sqvm, "weaponVar type is not float!");
		return SQRESULT_ERROR;
	}

	if (context == ScriptContext::SERVER)
	{
		CWeaponX* weapon = (CWeaponX*)ent;
		*(float*)(&weapon->weaponVars[varInfo->offset]) = value;
	}
	else // if (context == ScriptContext::CLIENT)
	{
		C_WeaponX* weapon = (C_WeaponX*)ent;
		*(float*)(&weapon->weaponVars[varInfo->offset]) = value;
	}

	return SQRESULT_NULL;
}

ON_DLL_LOAD_CLIENT("client.dll", ClWeaponVarArrayAddress, (CModule mod))
{
	weaponVarArray<ScriptContext::CLIENT> = (mod.Offset(0x942ca0).RCast<WeaponVarInfo*>());
	C_WeaponX_vftable = mod.Offset(0x998638).RCast<void*>();
	AUTOHOOK_DISPATCH();
}

ON_DLL_LOAD("server.dll", SvWeaponVarArrayAddress, (CModule mod))
{
	weaponVarArray<ScriptContext::SERVER> = (mod.Offset(0x997dc0).RCast<WeaponVarInfo*>());
	CWeaponX_vftable = mod.Offset(0x98E2B8).RCast<void*>();
}
// script try { ScriptWeaponVars_SetInt(__w(), eWeaponVar.ammo_clip_size, 50) } catch (ex) {print(ex)}
// script try { ScriptWeaponVars_SetFloat(__w(), eWeaponVar.spread_stand_hip, 10) } catch (ex) {print(ex)}
