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

enum GameState
{
	LOADING = 0,
	MAINMENU = 1,
	LOBBY = 2,
	INGAME = 3
};

enum PluginLoadDLL
{
	ENGINE = 0,
	CLIENT,
	SERVER
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
	sq_pushobjectType __sq_pushobject;

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
	sq_getfunctionType __sq_getfunction;

	sq_schedule_call_externalType __sq_schedule_call_external;
};

struct MessageSource
{
	const char* file;
	const char* func;
	int line;
};

// This is a stripped down version of spdlog::details::log_msg
// This is so that we can make it cross DLL boundaries
struct LogMsg
{
	int level;
	uint64_t timestamp;
	const char* msg;
	MessageSource source;
	int pluginHandle;
};

typedef void (*loggerfunc_t)(LogMsg* msg);
typedef void (*PLUGIN_RELAY_INVITE_TYPE)(const char* invite);

struct PluginNorthstarData
{
	const char* version;
	HMODULE northstarModule;
	int pluginHandle;
};

struct PluginInitFuncs
{
	loggerfunc_t logger;
	PLUGIN_RELAY_INVITE_TYPE relayInviteFunc;
};

struct PluginEngineData
{
	void* ConCommandConstructor;
	void* conVarMalloc;
	void* conVarRegister;
	void* ConVar_Vtable;
	void* IConVar_Vtable;
};

struct PluginGameStatePresence
{
	const char* id;
	const char* name;
	const char* description;
	const char* password;

	bool is_server;
	bool is_local;
	GameState state;

	const char* map;
	const char* map_displayname;
	const char* playlist;
	const char* playlist_displayname;

	int current_players;
	int max_players;

	int own_score;
	int other_highest_score; // NOTE: The highest score OR the second highest score if we have the highest
	int max_score;

	int timestamp_end;
};

/// <summary> Async communication within the plugin system
/// Due to the asynchronous nature of plugins, combined with the limitations of multi-compiler support
/// and the custom memory allocator used by r2, is it difficult to safely get data across DLL boundaries
/// from Northstar to plugin unless Northstar can get memory-clear signal back.
/// To do this, we use a request-response system
/// This means that if a plugin wants a piece of data, it will send a request to Northstar in the form of an
/// PLUGIN_REQUESTS_[DATA]_DATA call. The first argument to this call is a function pointer to call to return the data
/// Northstar will then, when possible, construct the requested data and call the function
/// This ensures that the process blocks until the data is ingested, and means it can safely be deleted afterwards without risk of dangling
/// pointers On the plugin side, the data should be ingested into a class guarded by mutexes The provided Plugin Library will handle all
/// this automatically.
/// </summary>

// Northstar -> Plugin
typedef void (*PLUGIN_INIT_TYPE)(PluginInitFuncs* funcs, PluginNorthstarData* data);
typedef void (*PLUGIN_INIT_SQVM_TYPE)(SquirrelFunctions* funcs);
typedef void (*PLUGIN_INFORM_SQVM_CREATED_TYPE)(ScriptContext context, CSquirrelVM* sqvm);
typedef void (*PLUGIN_INFORM_SQVM_DESTROYED_TYPE)(ScriptContext context);

// Async Communication types

// Northstar -> Plugin
typedef void (*PLUGIN_PUSH_PRESENCE_TYPE)(PluginGameStatePresence* data);
typedef void (*PLUGIN_INFORM_DLL_LOAD_TYPE)(PluginLoadDLL dll, void* data);
