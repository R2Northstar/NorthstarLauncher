
#include "client/weaponx.h"
#include "server/weaponx.h"
#include "modweaponvars.h"
#include "squirrel/squirrel.h"
#include "client/r2client.h"
#include "server/r2server.h"

typedef bool (*calculateWeaponValuesType)(int bitfield, void* weapon, void* weaponVarLocation);
typedef char* (*get2ndParamForRecalcModFuncType)(void* weapon);

template <ScriptContext context> get2ndParamForRecalcModFuncType get2ndParamForRecalcModFunc;
template <ScriptContext context> calculateWeaponValuesType _CalculateWeaponValues;
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

// RENAME THIS BEFORE MERGE (REVIEWERS IF YOU STILL SEE THIS BLOCK IT NOWNOWNOWNOWNOW)
AUTOHOOK(Cl_WeaponTick, client.dll + 0x59D1E0, bool, __fastcall, (C_WeaponX * weapon))
{
	SQObject* entInstance = g_pSquirrel<ScriptContext::CLIENT>->__sq_createscriptinstance(weapon);
	g_pSquirrel<ScriptContext::CLIENT>->Call("CodeCallback_PredictWeaponMods", entInstance);

	bool result = Cl_WeaponTick(weapon);

	return result;
}

// side note, may be an incorrect name
AUTOHOOK(CPlayerSimulate, server.dll + 0x5A6E50, bool, __fastcall, (CBasePlayer * player, int unk_1, bool unk_2))
{
	SQObject* entInstance = g_pSquirrel<ScriptContext::SERVER>->__sq_createscriptinstance(player);

	g_pSquirrel<ScriptContext::SERVER>->Call("CodeCallback_DoWeaponModsForPlayer", entInstance);
	bool result = CPlayerSimulate(player, unk_1, unk_2);
	g_pSquirrel<ScriptContext::SERVER>->Call("CodeCallback_DoWeaponModsForPlayer", entInstance);

	return result;
}

AUTOHOOK(Cl_CalcWeaponMods, client.dll + 0x3CA0B0, bool, __fastcall, (int mods, char* unk_1, char* weaponVars, bool unk_3, int unk_4))
{
	bool result;
	if (IsWeapon<ScriptContext::CLIENT>((void**)(weaponVars - 0x1700)))
	{
		SQObject* entInstance = g_pSquirrel<ScriptContext::CLIENT>->__sq_createscriptinstance((void**)(weaponVars - 0x1700));
		result = Cl_CalcWeaponMods(mods, unk_1, weaponVars, unk_3, unk_4);
		g_pSquirrel<ScriptContext::CLIENT>->Call("CodeCallback_ApplyModWeaponVars", entInstance);
	}
	else
		result = Cl_CalcWeaponMods(mods, unk_1, weaponVars, unk_3, unk_4);

	return result;
}

AUTOHOOK(Sv_CalcWeaponMods, server.dll + 0x6C8B80, bool, __fastcall, (int unk_0, char* unk_1, char* unk_2, bool unk_3, int unk_4))
{
	bool result = Sv_CalcWeaponMods(unk_0, unk_1, unk_2, unk_3, unk_4);

	if (result && IsWeapon<ScriptContext::SERVER>((void**)(unk_2 - 0x1410)))
	{
		SQObject* entInstance = g_pSquirrel<ScriptContext::SERVER>->__sq_createscriptinstance((void**)(unk_2 - 0x1410));
		g_pSquirrel<ScriptContext::SERVER>->Call("CodeCallback_ApplyModWeaponVars", entInstance);
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

ADD_SQFUNC("void", ModWeaponVars_SetFloat, "entity weapon, int weaponVar, float value", "", ScriptContext::SERVER | ScriptContext::CLIENT)
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
	int weaponVar = g_pSquirrel<context>->getinteger(sqvm, 2);
	bool value = g_pSquirrel<context>->getbool(sqvm, 3);
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

// SERVER only because client does this very often
ADD_SQFUNC("void", RecalculateModsForWeapon, "entity weapon", "", ScriptContext::SERVER | ScriptContext::CLIENT)
{
	void** ent = g_pSquirrel<context>->getentity<void*>(sqvm, 1);
	if (ent == nullptr)
	{
		g_pSquirrel<context>->raiseerror(sqvm, "Entity is not a weapon");
		return SQRESULT_ERROR;
	}
	if (!IsWeapon<context>(ent))
	{
		g_pSquirrel<context>->raiseerror(sqvm, "Entity is not a weapon");
		return SQRESULT_ERROR;
	}
	if (context == ScriptContext::SERVER)
	{
		CWeaponX* weapon = (CWeaponX*)ent;
		char* secondParamForCalcWeaponValues = get2ndParamForRecalcModFunc<context>(weapon);
		if (!_CalculateWeaponValues<context>(weapon->currentModBitfield, secondParamForCalcWeaponValues, (void*)&weapon->weaponVars))
		{
			g_pSquirrel<context>->raiseerror(sqvm, "Weapon var calculation failed...");
			return SQRESULT_ERROR;
		}
	}
	else
	{
		C_WeaponX* weapon = (C_WeaponX*)ent;
		char* secondParamForCalcWeaponValues = get2ndParamForRecalcModFunc<context>(weapon);
		if (!_CalculateWeaponValues<context>(weapon->currentModBitfield, secondParamForCalcWeaponValues, (void*)&weapon->weaponVars))
		{
			g_pSquirrel<context>->raiseerror(sqvm, "Weapon var calculation failed...");
			return SQRESULT_ERROR;
		}
	}

	return SQRESULT_NULL;
}

ON_DLL_LOAD_CLIENT("client.dll", ModWeaponVars_ClientInit, (CModule mod))
{
	const ScriptContext context = ScriptContext::CLIENT;
	weaponVarArray<context> = mod.Offset(0x942ca0).RCast<WeaponVarInfo*>();
	// requires client prediction, find a way to enforce that(?)
	_CalculateWeaponValues<context> = mod.Offset(0x3CA060).RCast<calculateWeaponValuesType>();
	get2ndParamForRecalcModFunc<context> = mod.Offset(0xBB4B0).RCast<get2ndParamForRecalcModFuncType>();
	C_WeaponX_vftable = mod.Offset(0x998638).RCast<void*>();
	AUTOHOOK_DISPATCH_MODULE(client.dll);
}

ON_DLL_LOAD("server.dll", ModWeaponVars_ServerInit, (CModule mod))
{
	const ScriptContext context = ScriptContext::SERVER;
	weaponVarArray<context> = mod.Offset(0x997dc0).RCast<WeaponVarInfo*>();
	_CalculateWeaponValues<context> = mod.Offset(0x6C8B30).RCast<calculateWeaponValuesType>();
	get2ndParamForRecalcModFunc<context> = mod.Offset(0xF0CD0).RCast<get2ndParamForRecalcModFuncType>();
	CWeaponX_vftable = mod.Offset(0x98E2B8).RCast<void*>();
	AUTOHOOK_DISPATCH_MODULE(server.dll);
}
// script try { ModWeaponVars_SetInt(__w(), eWeaponVar.ammo_clip_size, 50) } catch (ex) {print(ex)}
// script try { ModWeaponVars_SetFloat(__w(), eWeaponVar.spread_stand_hip, 10) } catch (ex) {print(ex)}
// script_client foreach (string k, int v in eWeaponVar) try { ModWeaponVars_SetFloat(__w(), eWeaponVar.spread_stand_hip,
// __w().GetWeaponSettingFloat(v)) } catch (e){}
