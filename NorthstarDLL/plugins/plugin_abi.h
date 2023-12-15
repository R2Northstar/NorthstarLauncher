#pragma once
#include "squirrel/squirrelclasstypes.h"
#include <stdint.h>

#define ABI_VERSION 4

// can't use bitwise ops on enum classes
namespace PluginContext
{
	enum : uint8_t
	{
		DEDICATED = 0x1,
		CLIENT = 0x2,
	};
}

enum ObjectType
{
	CONCOMMANDS = 0,
	CONVAR = 1,
};

extern "C"
{
	typedef void (*PLUGIN_RELAY_INVITE_TYPE)(const char* invite);
	typedef void* (*CreateObjectFunc)(ObjectType type);

	typedef void (*PluginFnCommandCallback_t)(void* command);
	typedef void (*PluginConCommandConstructorType)(
		void* newCommand, const char* name, PluginFnCommandCallback_t callback, const char* helpString, int flags, void* parent);
	typedef void (*PluginConVarRegisterType)(
		void* pConVar,
		const char* pszName,
		const char* pszDefaultValue,
		int nFlags,
		const char* pszHelpString,
		bool bMin,
		float fMin,
		bool bMax,
		float fMax,
		void* pCallback);
	typedef void (*PluginConVarMallocType)(void* pConVarMaloc, int a2, int a3);
}

struct PluginNorthstarData
{
	const int pluginHandle;
};

struct PluginEngineData
{
	PluginConCommandConstructorType ConCommandConstructor;
	PluginConVarMallocType conVarMalloc;
	PluginConVarRegisterType conVarRegister;
	void* ConVar_Vtable;
	void* IConVar_Vtable;
	void* g_pCVar;
};

/// <summary> Async communication within the plugin system
/// Due to the asynchronous nature of plugins, combined with the limitations of multi-compiler support
/// and the custom memory allocator used by r2, is it difficult to safely get data across DLL boundaries
/// from Northstar to plugin unless Northstar can own that memory.
/// This means that plugins should manage their own memory and can only receive data from northstar using one of the functions below.
/// These should be exports of the plugin DLL. If they are not exported, they will not be called.
/// Note that it is not required to have these exports if you do not use them.
/// </summary>

// Northstar -> Plugin

