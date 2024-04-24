
#include "modweaponvars.h"
#include "squirrel/squirrel.h"
#include "client/r2client.h"
#include "server/r2server.h"

template <ScriptContext context> WeaponVarInfo* weaponVarArray;

AUTOHOOK_INIT()

// TODO: _actually_ reverse this.
// Seemingly recalculates weapon variables every tick, figure out what this actually does
// rn we are "disabling" this function because it seems to make stuff
// work, figure out why.
// also ""slight"" performance increase
AUTOHOOK(Cl_UpdateWeaponVars, client.dll + 0x5B2670, char, __fastcall, (void* a2))
{
	return '\0';
}

ADD_SQFUNC("void", ScriptWeaponVars_SetInt, "entity weapon, int weaponVar, int value", "", ScriptContext::SERVER | ScriptContext::CLIENT)
{
	char* ent = g_pSquirrel<context>->getentity<char>(sqvm, 1);
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
		*(int*)(ent + 0x1410 + varInfo->offset) = value;
	}
	else // if (context == ScriptContext::CLIENT)
	{
		*(int*)(ent + 0x1700 + varInfo->offset) = value;
	}

	return SQRESULT_NULL;
}

ADD_SQFUNC(
	"void", ScriptWeaponVars_SetFloat, "entity weapon, int weaponVar, float value", "", ScriptContext::SERVER | ScriptContext::CLIENT)
{
	char* ent = g_pSquirrel<context>->getentity<char>(sqvm, 1);
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
		*(float*)(ent + 0x1410 + varInfo->offset) = value;
	}
	else // if (context == ScriptContext::CLIENT)
	{
		*(float*)(ent + 0x1700 + varInfo->offset) = value;
	}

	return SQRESULT_NULL;
}

ON_DLL_LOAD_CLIENT("client.dll", ClWeaponVarArrayAddress, (CModule mod))
{
	weaponVarArray<ScriptContext::CLIENT> = (mod.Offset(0x942ca0).RCast<WeaponVarInfo*>());
	AUTOHOOK_DISPATCH();
}

ON_DLL_LOAD("server.dll", SvWeaponVarArrayAddress, (CModule mod))
{
	weaponVarArray<ScriptContext::SERVER> = (mod.Offset(0x997dc0).RCast<WeaponVarInfo*>());
}
// script try { ScriptWeaponVars_SetInt(__w(), eWeaponVar.ammo_clip_size, 50) } catch (ex) {print(ex)}
// script try { ScriptWeaponVars_SetFloat(__w(), eWeaponVar.spread_stand_hip, 10) } catch (ex) {print(ex)}
