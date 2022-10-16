#pragma once
#include <string>
#include "squirrelclasstypes.h"

#define ABI_VERSION 2
/// <summary>
/// This enum is used for referencing the different types of objects we can pass to a plugin
/// Anything exposed to a plugin must not a be C++ type, as they could break when compiling with a different compiler.
/// Any ABI incompatible change must increment the version number.
/// Nothing must be removed from this enum, only appended. When it absolutely necessary to deprecate an object, it should return UNSUPPORTED
/// when retrieved
/// </summary>
enum PluginObject
{
	UNSUPPORTED = 0,
	SQUIRREL = 1,
	DUMMY = 0xFFFF
};

struct SquirrelFunctions
{
	RegisterSquirrelFuncType RegisterSquirrelFunc;
	sq_defconstType __sq_defconst;

	sq_compilebufferType __sq_compilebuffer;
	sq_callType __sq_call;
	sq_raiseerrorType __sq_raiseerror;

	sq_newarrayType __sq_newarray;
	sq_arrayappendType __sq_arrayappend;

	sq_newtableType __sq_newtable;
	sq_newslotType __sq_newslot;

	sq_pushroottableType __sq_pushroottable;
	sq_pushstringType __sq_pushstring;
	sq_pushintegerType __sq_pushinteger;
	sq_pushfloatType __sq_pushfloat;
	sq_pushboolType __sq_pushbool;
	sq_pushassetType __sq_pushasset;
	sq_pushvectorType __sq_pushvector;
	sq_pushSQObjectType __sq_pushSQObject;

	sq_getstringType __sq_getstring;
	sq_getintegerType __sq_getinteger;
	sq_getfloatType __sq_getfloat;
	sq_getboolType __sq_getbool;
	sq_getType __sq_get;
	sq_getassetType __sq_getasset;
	sq_getuserdataType __sq_getuserdata;
	sq_getvectorType __sq_getvector;

	sq_createuserdataType __sq_createuserdata;
	sq_setuserdatatypeidType __sq_setuserdatatypeid;
	sq_getSquirrelFunctionType __sq_getSquirrelFunction;

	sq_schedule_call_externalType __sq_schedule_call_external;
};

struct MessageSource {
	const char* file;
	const char* func;
	int line;
};

// This is a stripped down version of spdlog::details::log_msg
// This is so that we can make it cross DLL boundaries
struct LogMsg {
	int level;
	uint64_t timestamp;
	const char* msg;
	MessageSource source;
};

typedef void (*loggerfunc_t)(LogMsg* msg);

struct PluginInitFuncs
{
	loggerfunc_t logger;
};

typedef void (*PLUGIN_INIT_TYPE)(PluginInitFuncs* funcs);
typedef void (*PLUGIN_INIT_SQVM_TYPE)(SquirrelFunctions* funcs);
typedef void (*PLUGIN_INFORM_SQVM_CREATED_TYPE)(ScriptContext context, CSquirrelVM* sqvm);
