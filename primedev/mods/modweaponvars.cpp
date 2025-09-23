
#include "client/weaponx.h"
#include "server/weaponx.h"
#include "modweaponvars.h"
#include "squirrel/squirrel.h"
#include "client/r2client.h"
#include "server/r2server.h"

std::map<size_t, std::string> sv_modWeaponVarStrings;
std::map<size_t, std::string> cl_modWeaponVarStrings;
std::hash<std::string> hasher;

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

// Names might be wrong? Correct if needed
AUTOHOOK(CWeaponX__RegenerateAmmo, server.dll + 0x69E7A0, int, , (CWeaponX * self, CBasePlayer* player, int offhandSlot))
{
	SQObject* entInstance = g_pSquirrel<ScriptContext::SERVER>->__sq_createscriptinstance(self);

	g_pSquirrel<ScriptContext::SERVER>->Call("CodeCallback_DoWeaponModsForPlayer", entInstance);
	return CWeaponX__RegenerateAmmo(self, player, offhandSlot);
}

AUTOHOOK(C_WeaponX__RegenerateAmmo, client.dll + 0x5B6020, int, , (C_WeaponX * self, CBasePlayer* player, int offhandSlot))
{
	SQObject* entInstance = g_pSquirrel<ScriptContext::CLIENT>->__sq_createscriptinstance(self);

	g_pSquirrel<ScriptContext::CLIENT>->Call("CodeCallback_PredictWeaponMods", entInstance);
	return C_WeaponX__RegenerateAmmo(self, player, offhandSlot);
}

bool IsBadReadPtr(void* p)
{
	MEMORY_BASIC_INFORMATION mbi = {0};
	if (::VirtualQuery(p, &mbi, sizeof(mbi)))
	{
		DWORD mask =
			(PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);
		bool b = !(mbi.Protect & mask);
		// check the page is not a guard page
		if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS))
			b = true;

		return b;
	}
	return true;
}

AUTOHOOK(
	C_WeaponX__CalcWeaponMods, client.dll + 0x3CA0B0, bool, __fastcall, (int mods, char* unk_1, char* weaponVars, bool unk_3, int unk_4))
{
	bool result;

	if (IsBadReadPtr(weaponVars - offsetof(C_WeaponX, weaponVars)))
		return C_WeaponX__CalcWeaponMods(mods, unk_1, weaponVars, unk_3, unk_4);

	if (IsWeapon<ScriptContext::CLIENT>((void**)(weaponVars - offsetof(C_WeaponX, weaponVars))))
	{
		result = C_WeaponX__CalcWeaponMods(mods, unk_1, weaponVars, unk_3, unk_4);
		SQObject* entInstance =
			g_pSquirrel<ScriptContext::CLIENT>->__sq_createscriptinstance((void**)(weaponVars - offsetof(C_WeaponX, weaponVars)));
		g_pSquirrel<ScriptContext::CLIENT>->Call("CodeCallback_ApplyModWeaponVars", entInstance);
	}
	else
	{
		return C_WeaponX__CalcWeaponMods(mods, unk_1, weaponVars, unk_3, unk_4);
	}

	return result;
}

AUTOHOOK(
	CWeaponX__CalcWeaponMods, server.dll + 0x6C8B80, bool, __fastcall, (int unk_0, char* unk_1, char* weaponVars, bool unk_3, int unk_4))
{
	bool result = CWeaponX__CalcWeaponMods(unk_0, unk_1, weaponVars, unk_3, unk_4);

	if (IsBadReadPtr(weaponVars - offsetof(CWeaponX, weaponVars)))
		return result;

	if (result && IsWeapon<ScriptContext::SERVER>((void**)(weaponVars - offsetof(CWeaponX, weaponVars))))
	{
		SQObject* entInstance =
			g_pSquirrel<ScriptContext::SERVER>->__sq_createscriptinstance((void**)(weaponVars - offsetof(CWeaponX, weaponVars)));
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

ADD_SQFUNC("int", ModWeaponVars_GetType, "int eWeaponVar", "", ScriptContext::SERVER | ScriptContext::CLIENT)
{
	int weaponVar = g_pSquirrel<context>->getinteger(sqvm, 1);

	if (weaponVar < 1 || weaponVar > WEAPON_VAR_COUNT) // weapon vars start at one and end at 725 inclusive
	{
		// invalid weapon var index
		g_pSquirrel<context>->raiseerror(sqvm, "Invalid eWeaponVar!");
		return SQRESULT_ERROR;
	}
	WeaponVarInfo* varInfo = &weaponVarArray<context>[weaponVar];
	g_pSquirrel<context>->pushinteger(sqvm, varInfo->type);

	return SQRESULT_NOTNULL;
}

ADD_SQFUNC("void", ModWeaponVars_SetString, "entity weapon, int weaponVar, string value", "", ScriptContext::SERVER | ScriptContext::CLIENT)
{
	void** ent = g_pSquirrel<context>->getentity<void*>(sqvm, 1);
	if (!IsWeapon<context>(ent))
	{
		g_pSquirrel<context>->raiseerror(sqvm, "Entity is not a weapon");
		return SQRESULT_ERROR;
	}
	int weaponVar = g_pSquirrel<context>->getinteger(sqvm, 2);
	std::string value = g_pSquirrel<context>->getstring(sqvm, 3);
	if (weaponVar < 1 || weaponVar > WEAPON_VAR_COUNT) // weapon vars start at one and end at 725 inclusive
	{
		// invalid weapon var index
		g_pSquirrel<context>->raiseerror(sqvm, "Invalid eWeaponVar!");
		return SQRESULT_ERROR;
	}

	const WeaponVarInfo* varInfo = &weaponVarArray<context>[weaponVar];

	if (varInfo->type != WVT_STRING)
	{
		// invalid type used
		g_pSquirrel<context>->raiseerror(sqvm, "eWeaponVar is not a string!");
		return SQRESULT_ERROR;
	}

	size_t valueHash = hasher(value);
	if (context == ScriptContext::SERVER)
	{
		if (sv_modWeaponVarStrings.find(valueHash) == sv_modWeaponVarStrings.end())
			sv_modWeaponVarStrings.emplace(valueHash, value);
		CWeaponX* weapon = (CWeaponX*)ent;
		*(const char**)(&weapon->weaponVars[varInfo->offset]) = sv_modWeaponVarStrings[valueHash].c_str();
	}
	else // if (context == ScriptContext::CLIENT)
	{
		if (cl_modWeaponVarStrings.find(valueHash) == cl_modWeaponVarStrings.end())
			cl_modWeaponVarStrings.emplace(valueHash, value);

		C_WeaponX* weapon = (C_WeaponX*)ent;
		*(const char**)(&weapon->weaponVars[varInfo->offset]) = cl_modWeaponVarStrings[valueHash].c_str();
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
ADD_SQFUNC("void", ModWeaponVars_CalculateWeaponMods, "entity weapon", "", ScriptContext::SERVER | ScriptContext::CLIENT)
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

ON_DLL_LOAD("server.dll", ModWeaponVars_ServerInit, (CModule mod))
{
	const ScriptContext context = ScriptContext::SERVER;
	weaponVarArray<context> = mod.Offset(0x997dc0).RCast<WeaponVarInfo*>();
	_CalculateWeaponValues<context> = mod.Offset(0x6C8B30).RCast<calculateWeaponValuesType>();
	get2ndParamForRecalcModFunc<context> = mod.Offset(0xF0CD0).RCast<get2ndParamForRecalcModFuncType>();
	CWeaponX_vftable = mod.Offset(0x98E2B8).RCast<void*>();
	AUTOHOOK_DISPATCH_MODULE(server.dll);
}

ON_DLL_LOAD_CLIENT("client.dll", ModWeaponVars_ClientInit, (CModule mod))
{
	const ScriptContext context = ScriptContext::CLIENT;
	weaponVarArray<context> = mod.Offset(0x942ca0).RCast<WeaponVarInfo*>();
	_CalculateWeaponValues<context> = mod.Offset(0x3CA060).RCast<calculateWeaponValuesType>();
	get2ndParamForRecalcModFunc<context> = mod.Offset(0xBB4B0).RCast<get2ndParamForRecalcModFuncType>();
	C_WeaponX_vftable = mod.Offset(0x998638).RCast<void*>();
	AUTOHOOK_DISPATCH_MODULE(client.dll);
}
