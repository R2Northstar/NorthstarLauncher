
#include "client/weaponx.h"
#include "server/weaponx.h"
#include "modweaponvars.h"
#include "squirrel/squirrel.h"
#include "client/r2client.h"
#include "server/r2server.h"

template <ScriptContext context> WeaponVarInfo* weaponVarArray;
template <ScriptContext context> std::vector<SQObject> weaponModCallbacks;

template <ScriptContext context> bool IsWeapon(void** ent)
{
	if (context == ScriptContext::SERVER)
		return *ent == CWeaponX_vftable;
	else
		return *ent == C_WeaponX_vftable;
}

AUTOHOOK_INIT()

AUTOHOOK(Cl_CalcWeaponMods, client.dll + 0x3CA0B0, bool, __fastcall, (int unk_0, char* unk_1, char* unk_2, bool unk_3, int unk_4))
{
	bool result = Cl_CalcWeaponMods(unk_0, unk_1, unk_2, unk_3, unk_4);

	if (IsWeapon<ScriptContext::CLIENT>((void**)(unk_2 - 0x1700)))
	{
		// TODO: codecallback
	}

	return result;
}

AUTOHOOK(Sv_CalcWeaponMods, server.dll + 0x6C8B80, bool, __fastcall, (int unk_0, char* unk_1, char* unk_2, bool unk_3, int unk_4))
{
	bool result = Sv_CalcWeaponMods(unk_0, unk_1, unk_2, unk_3, unk_4);

	if (IsWeapon<ScriptContext::SERVER>((void**)(unk_2 - 0x1410)))
	{
		// TODO: codecallback
	}

	return result;
}

ADD_SQFUNC("void", ModWeaponVars_SetInt, "entity weapon, int weaponVar, int value", "", ScriptContext::SERVER | ScriptContext::CLIENT)
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
	if (varInfo->type != WVT_INTEGER)
	{
		// invalid type used
		g_pSquirrel<context>->raiseerror(sqvm, "eWeaponVar is not an integer!");
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
	"void", ModWeaponVars_SetFloat, "entity weapon, int weaponVar, float value", "", ScriptContext::SERVER | ScriptContext::CLIENT)
{
	void** ent = g_pSquirrel<context>->getentity<void*>(sqvm, 1);
	if (!IsWeapon<context>(ent))
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
	if (varInfo->type != WVT_FLOAT32)
	{
		// invalid type used
		g_pSquirrel<context>->raiseerror(sqvm, "eWeaponVar is not a float!");
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
ADD_SQFUNC("void", ModWeaponVars_SetBool, "entity weapon, int weaponVar, bool value", "", ScriptContext::SERVER | ScriptContext::CLIENT)
{
	void** ent = g_pSquirrel<context>->getentity<void*>(sqvm, 1);
	if (!IsWeapon<context>(ent))
	{
		g_pSquirrel<context>->raiseerror(sqvm, "Entity is not a weapon");
		return SQRESULT_ERROR;
	}
	bool weaponVar = g_pSquirrel<context>->getbool(sqvm, 2);
	float value = g_pSquirrel<context>->getfloat(sqvm, 3);
	if (weaponVar < 1 || weaponVar > WEAPON_VAR_COUNT) // weapon vars start at one and end at 725 inclusive
	{
		// invalid weapon var index
		g_pSquirrel<context>->raiseerror(sqvm, "Invalid eWeaponVar!");
		return SQRESULT_ERROR;
	}

	WeaponVarInfo* varInfo = &weaponVarArray<context>[weaponVar];
	if (varInfo->type != WVT_BOOLEAN)
	{
		// invalid type used
		g_pSquirrel<context>->raiseerror(sqvm, "eWeaponVar is not a boolean!");
		return SQRESULT_ERROR;
	}

	if (context == ScriptContext::SERVER)
	{
		CWeaponX* weapon = (CWeaponX*)ent;
		*(bool*)(&weapon->weaponVars[varInfo->offset]) = value;
	}
	else // if (context == ScriptContext::CLIENT)
	{
		C_WeaponX* weapon = (C_WeaponX*)ent;
		*(bool*)(&weapon->weaponVars[varInfo->offset]) = value;
	}

	return SQRESULT_NULL;
}

ADD_SQFUNC("void", AddCallback_ApplyModWeaponVars, "void functionref( entity ) callback, int priority", "", ScriptContext::CLIENT | ScriptContext::SERVER)
{
	return SQRESULT_NULL;
}

ON_DLL_LOAD_CLIENT("client.dll", ModWeaponVars_ClientInit, (CModule mod))
{
	weaponVarArray<ScriptContext::CLIENT> = (mod.Offset(0x942ca0).RCast<WeaponVarInfo*>());
	C_WeaponX_vftable = mod.Offset(0x998638).RCast<void*>();
	AUTOHOOK_DISPATCH_MODULE(client.dll);
}

ON_DLL_LOAD("server.dll", ModWeaponVars_ServerInit, (CModule mod))
{
	weaponVarArray<ScriptContext::SERVER> = (mod.Offset(0x997dc0).RCast<WeaponVarInfo*>());
	CWeaponX_vftable = mod.Offset(0x98E2B8).RCast<void*>();
	AUTOHOOK_DISPATCH_MODULE(server.dll);
}
// script try { ModWeaponVars_SetInt(__w(), eWeaponVar.ammo_clip_size, 50) } catch (ex) {print(ex)}
// script try { ModWeaponVars_SetFloat(__w(), eWeaponVar.spread_stand_hip, 10) } catch (ex) {print(ex)}
// script_client foreach (string k, int v in eWeaponVar) try { ModWeaponVars_SetFloat(__w(), eWeaponVar.spread_stand_hip,
// __w().GetWeaponSettingFloat(v)) } catch (e){}
