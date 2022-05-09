#include "pch.h"
#include "squirrel.h"
#include "hooks.h"
#include "hookutils.h"
#include "sigscanning.h"
#include "concommand.h"
#include "modmanager.h"
#include <iostream>

// hook forward declarations
typedef SQInteger (*SQPrintType)(void* sqvm, char* fmt, ...);
SQPrintType ClientSQPrint;
SQPrintType UISQPrint;
SQPrintType ServerSQPrint;
template <ScriptContext context> SQInteger SQPrintHook(void* sqvm, char* fmt, ...);

typedef void* (*CreateNewVMType)(void* a1, ScriptContext contextArg);
CreateNewVMType ClientCreateNewVM; // only need a client one since ui doesn't have its own func for this
CreateNewVMType ServerCreateNewVM;
template <ScriptContext context> void* CreateNewVMHook(void* a1, ScriptContext contextArg);

typedef void (*DestroyVMType)(void* a1, void* sqvm);
DestroyVMType ClientDestroyVM; // only need a client one since ui doesn't have its own func for this
DestroyVMType ServerDestroyVM;
template <ScriptContext context> void DestroyVMHook(void* a1, void* sqvm);

typedef void (*ScriptCompileError)(void* sqvm, const char* error, const char* file, int line, int column);
ScriptCompileError ClientSQCompileError; // only need a client one since ui doesn't have its own func for this
ScriptCompileError ServerSQCompileError;
template <ScriptContext context> void ScriptCompileErrorHook(void* sqvm, const char* error, const char* file, int line, int column);

typedef char (*CallScriptInitCallbackType)(void* sqvm, const char* callback);
CallScriptInitCallbackType ClientCallScriptInitCallback;
CallScriptInitCallbackType ServerCallScriptInitCallback;
template <ScriptContext context> char CallScriptInitCallbackHook(void* sqvm, const char* callback);

RegisterSquirrelFuncType ClientRegisterSquirrelFunc;
RegisterSquirrelFuncType ServerRegisterSquirrelFunc;

template <ScriptContext context> void ExecuteCodeCommand(const CCommand& args);

// inits
SquirrelManager<ScriptContext::CLIENT>* g_ClientSquirrelManager;
SquirrelManager<ScriptContext::SERVER>* g_ServerSquirrelManager;
SquirrelManager<ScriptContext::UI>* g_UISquirrelManager;

ON_DLL_LOAD_RELIESON("client.dll", ClientSquirrel, ConCommand, (HMODULE baseAddress)
{
	HookEnabler hook;

	// client inits
	g_ClientSquirrelManager = new SquirrelManager<ScriptContext::CLIENT>();

	ENABLER_CREATEHOOK(
		hook,
		(char*)baseAddress + 0x12B00,
		&SQPrintHook<ScriptContext::CLIENT>,
		reinterpret_cast<LPVOID*>(&ClientSQPrint)); // client print function
	RegisterConCommand(
		"script_client", ExecuteCodeCommand<ScriptContext::CLIENT>, "Executes script code on the client vm", FCVAR_CLIENTDLL);

	// ui inits
	g_UISquirrelManager = new SquirrelManager<ScriptContext::UI>();

	ENABLER_CREATEHOOK(
		hook, (char*)baseAddress + 0x12BA0, &SQPrintHook<ScriptContext::UI>, reinterpret_cast<LPVOID*>(&UISQPrint)); // ui print function
	RegisterConCommand("script_ui", ExecuteCodeCommand<ScriptContext::UI>, "Executes script code on the ui vm", FCVAR_CLIENTDLL);

	ClientRegisterSquirrelFunc = (RegisterSquirrelFuncType)((char*)baseAddress + 0x108E0);

	g_ClientSquirrelManager->sq_compilebuffer = (sq_compilebufferType)((char*)baseAddress + 0x3110);
	g_UISquirrelManager->sq_compilebuffer = (sq_compilebufferType)((char*)baseAddress + 0x3110);
	g_ClientSquirrelManager->sq_pushroottable = (sq_pushroottableType)((char*)baseAddress + 0x5860);
	g_UISquirrelManager->sq_pushroottable = (sq_pushroottableType)((char*)baseAddress + 0x5860);

	g_ClientSquirrelManager->sq_call = (sq_callType)((char*)baseAddress + 0x8650);
	g_UISquirrelManager->sq_call = (sq_callType)((char*)baseAddress + 0x8650);

	g_ClientSquirrelManager->sq_newarray = (sq_newarrayType)((char*)baseAddress + 0x39F0);
	g_UISquirrelManager->sq_newarray = (sq_newarrayType)((char*)baseAddress + 0x39F0);
	g_ClientSquirrelManager->sq_arrayappend = (sq_arrayappendType)((char*)baseAddress + 0x3C70);
	g_UISquirrelManager->sq_arrayappend = (sq_arrayappendType)((char*)baseAddress + 0x3C70);

	g_ClientSquirrelManager->sq_pushstring = (sq_pushstringType)((char*)baseAddress + 0x3440);
	g_UISquirrelManager->sq_pushstring = (sq_pushstringType)((char*)baseAddress + 0x3440);
	g_ClientSquirrelManager->sq_pushinteger = (sq_pushintegerType)((char*)baseAddress + 0x36A0);
	g_UISquirrelManager->sq_pushinteger = (sq_pushintegerType)((char*)baseAddress + 0x36A0);
	g_ClientSquirrelManager->sq_pushfloat = (sq_pushfloatType)((char*)baseAddress + 0x3800);
	g_UISquirrelManager->sq_pushfloat = (sq_pushfloatType)((char*)baseAddress + 0x3800);
	g_ClientSquirrelManager->sq_pushbool = (sq_pushboolType)((char*)baseAddress + 0x3710);
	g_UISquirrelManager->sq_pushbool = (sq_pushboolType)((char*)baseAddress + 0x3710);
	g_ClientSquirrelManager->sq_raiseerror = (sq_raiseerrorType)((char*)baseAddress + 0x8470);
	g_UISquirrelManager->sq_raiseerror = (sq_raiseerrorType)((char*)baseAddress + 0x8470);

	g_ClientSquirrelManager->sq_getstring = (sq_getstringType)((char*)baseAddress + 0x60C0);
	g_UISquirrelManager->sq_getstring = (sq_getstringType)((char*)baseAddress + 0x60C0);
	g_ClientSquirrelManager->sq_getinteger = (sq_getintegerType)((char*)baseAddress + 0x60E0);
	g_UISquirrelManager->sq_getinteger = (sq_getintegerType)((char*)baseAddress + 0x60E0);
	g_ClientSquirrelManager->sq_getfloat = (sq_getfloatType)((char*)baseAddress + 0x6100);
	g_UISquirrelManager->sq_getfloat = (sq_getfloatType)((char*)baseAddress + 0x6100);
	g_ClientSquirrelManager->sq_getbool = (sq_getboolType)((char*)baseAddress + 0x6130);
	g_UISquirrelManager->sq_getbool = (sq_getboolType)((char*)baseAddress + 0x6130);
	g_ClientSquirrelManager->sq_get = (sq_getType)((char*)baseAddress + 0x7C30);
	g_UISquirrelManager->sq_get = (sq_getType)((char*)baseAddress + 0x7C30);

	ENABLER_CREATEHOOK(
		hook,
		(char*)baseAddress + 0x26130,
		&CreateNewVMHook<ScriptContext::CLIENT>,
		reinterpret_cast<LPVOID*>(&ClientCreateNewVM)); // client createnewvm function
	ENABLER_CREATEHOOK(
		hook,
		(char*)baseAddress + 0x26E70,
		&DestroyVMHook<ScriptContext::CLIENT>,
		reinterpret_cast<LPVOID*>(&ClientDestroyVM)); // client destroyvm function
	ENABLER_CREATEHOOK(
		hook,
		(char*)baseAddress + 0x79A50,
		&ScriptCompileErrorHook<ScriptContext::CLIENT>,
		reinterpret_cast<LPVOID*>(&ClientSQCompileError)); // client compileerror function
	ENABLER_CREATEHOOK(
		hook,
		(char*)baseAddress + 0x10190,
		&CallScriptInitCallbackHook<ScriptContext::CLIENT>,
		reinterpret_cast<LPVOID*>(&ClientCallScriptInitCallback)); // client callscriptinitcallback function
})

ON_DLL_LOAD_RELIESON("server.dll", ServerSquirrel, ConCommand, (HMODULE baseAddress)
{
	g_ServerSquirrelManager = new SquirrelManager<ScriptContext::SERVER>();

	ServerRegisterSquirrelFunc = (RegisterSquirrelFuncType)((char*)baseAddress + 0x1DD10);

	g_ServerSquirrelManager->sq_compilebuffer = (sq_compilebufferType)((char*)baseAddress + 0x3110);
	g_ServerSquirrelManager->sq_pushroottable = (sq_pushroottableType)((char*)baseAddress + 0x5840);
	g_ServerSquirrelManager->sq_call = (sq_callType)((char*)baseAddress + 0x8620);

	g_ServerSquirrelManager->sq_newarray = (sq_newarrayType)((char*)baseAddress + 0x39F0);
	g_ServerSquirrelManager->sq_arrayappend = (sq_arrayappendType)((char*)baseAddress + 0x3C70);

	g_ServerSquirrelManager->sq_pushstring = (sq_pushstringType)((char*)baseAddress + 0x3440);
	g_ServerSquirrelManager->sq_pushinteger = (sq_pushintegerType)((char*)baseAddress + 0x36A0);
	g_ServerSquirrelManager->sq_pushfloat = (sq_pushfloatType)((char*)baseAddress + 0x3800);
	g_ServerSquirrelManager->sq_pushbool = (sq_pushboolType)((char*)baseAddress + 0x3710);
	g_ServerSquirrelManager->sq_raiseerror = (sq_raiseerrorType)((char*)baseAddress + 0x8440);

	g_ServerSquirrelManager->sq_getstring = (sq_getstringType)((char*)baseAddress + 0x60A0);
	g_ServerSquirrelManager->sq_getinteger = (sq_getintegerType)((char*)baseAddress + 0x60C0);
	g_ServerSquirrelManager->sq_getfloat = (sq_getfloatType)((char*)baseAddress + 0x60E0);
	g_ServerSquirrelManager->sq_getbool = (sq_getboolType)((char*)baseAddress + 0x6110);
	g_ServerSquirrelManager->sq_get = (sq_getType)((char*)baseAddress + 0x7C00);

	HookEnabler hook;
	ENABLER_CREATEHOOK(
		hook,
		(char*)baseAddress + 0x1FE90,
		&SQPrintHook<ScriptContext::SERVER>,
		reinterpret_cast<LPVOID*>(&ServerSQPrint)); // server print function
	ENABLER_CREATEHOOK(
		hook,
		(char*)baseAddress + 0x260E0,
		&CreateNewVMHook<ScriptContext::SERVER>,
		reinterpret_cast<LPVOID*>(&ServerCreateNewVM)); // server createnewvm function
	ENABLER_CREATEHOOK(
		hook,
		(char*)baseAddress + 0x26E20,
		&DestroyVMHook<ScriptContext::SERVER>,
		reinterpret_cast<LPVOID*>(&ServerDestroyVM)); // server destroyvm function
	ENABLER_CREATEHOOK(
		hook,
		(char*)baseAddress + 0x799E0,
		&ScriptCompileErrorHook<ScriptContext::SERVER>,
		reinterpret_cast<LPVOID*>(&ServerSQCompileError)); // server compileerror function
	ENABLER_CREATEHOOK(
		hook,
		(char*)baseAddress + 0x1D5C0,
		&CallScriptInitCallbackHook<ScriptContext::SERVER>,
		reinterpret_cast<LPVOID*>(&ServerCallScriptInitCallback)); // server callscriptinitcallback function

	// cheat and clientcmd_can_execute allows clients to execute this, but since it's unsafe we only allow it when cheats are enabled
	// for script_client and script_ui, we don't use cheats, so clients can execute them on themselves all they want
	RegisterConCommand(
		"script",
		ExecuteCodeCommand<ScriptContext::SERVER>,
		"Executes script code on the server vm",
		FCVAR_GAMEDLL | FCVAR_CLIENTCMD_CAN_EXECUTE | FCVAR_CHEAT);
})

// hooks
template <ScriptContext context> SQInteger SQPrintHook(void* sqvm, char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);

	SQChar buf[1024];
	int charsWritten = vsnprintf_s(buf, _TRUNCATE, fmt, va);

	if (charsWritten > 0)
	{
		if (buf[charsWritten - 1] == '\n')
			buf[charsWritten - 1] = '\0';

		spdlog::info("[{} SCRIPT] {}", GetContextName(context), buf);
	}

	va_end(va);
	return 0;
}

template <ScriptContext context> void* CreateNewVMHook(void* a1, ScriptContext realContext)
{
	void* sqvm;

	if (context == ScriptContext::CLIENT)
	{
		sqvm = ClientCreateNewVM(a1, realContext);

		if (realContext == ScriptContext::UI)
			g_UISquirrelManager->VMCreated(sqvm);
		else
			g_ClientSquirrelManager->VMCreated(sqvm);
	}
	else if (context == ScriptContext::SERVER)
	{
		sqvm = ServerCreateNewVM(a1, context);
		g_ServerSquirrelManager->VMCreated(sqvm);
	}

	spdlog::info("CreateNewVM {} {}", GetContextName(realContext), sqvm);
	return sqvm;
}

template <ScriptContext context> void DestroyVMHook(void* a1, void* sqvm)
{
	ScriptContext realContext = context; // ui and client use the same function so we use this for prints

	if (context == ScriptContext::CLIENT)
	{
		if (g_ClientSquirrelManager->sqvm == sqvm)
			g_ClientSquirrelManager->VMDestroyed();
		else if (g_UISquirrelManager->sqvm == sqvm)
		{
			g_UISquirrelManager->VMDestroyed();
			realContext = ScriptContext::UI;
		}

		ClientDestroyVM(a1, sqvm);
	}
	else if (context == ScriptContext::SERVER)
	{
		g_ServerSquirrelManager->VMDestroyed();
		ServerDestroyVM(a1, sqvm);
	}

	spdlog::info("DestroyVM {} {}", GetContextName(realContext), sqvm);
}

template <ScriptContext context> void ScriptCompileErrorHook(void* sqvm, const char* error, const char* file, int line, int column)
{
	ScriptContext realContext = context; // ui and client use the same function so we use this for prints
	if (context == ScriptContext::CLIENT && sqvm == g_UISquirrelManager->sqvm)
		realContext = ScriptContext::UI;

	spdlog::error("{} SCRIPT COMPILE ERROR {}", GetContextName(realContext), error);
	spdlog::error("{} line [{}] column [{}]", file, line, column);

	// dont call the original since it kills game
	// in the future it'd be nice to do an actual error with UICodeCallback_ErrorDialog here, but only if we're compiling level scripts
	// compilestring and stuff shouldn't tho
	// though, that also has potential to be REALLY bad if we're compiling ui scripts lol
}

template <ScriptContext context> char CallScriptInitCallbackHook(void* sqvm, const char* callback)
{
	char ret;

	if (context == ScriptContext::CLIENT)
	{
		ScriptContext realContext = context; // ui and client use the same function so we use this for prints
		bool shouldCallCustomCallbacks = false;

		// since we don't hook arbitrary callbacks yet, make sure we're only doing callbacks on inits
		if (!strcmp(callback, "UICodeCallback_UIInit"))
		{
			realContext = ScriptContext::UI;
			shouldCallCustomCallbacks = true;
		}
		else if (!strcmp(callback, "ClientCodeCallback_MapSpawn"))
			shouldCallCustomCallbacks = true;

		// run before callbacks
		// todo: we need to verify if RunOn is valid for current state before calling callbacks
		if (shouldCallCustomCallbacks)
		{
			for (Mod mod : g_ModManager->m_loadedMods)
			{
				if (!mod.Enabled)
					continue;

				for (ModScript script : mod.Scripts)
				{
					for (ModScriptCallback modCallback : script.Callbacks)
					{
						if (modCallback.Context == realContext && modCallback.BeforeCallback.length())
						{
							spdlog::info(
								"Running custom {} script callback \"{}\"", GetContextName(realContext), modCallback.BeforeCallback);
							ClientCallScriptInitCallback(sqvm, modCallback.BeforeCallback.c_str());
						}
					}
				}
			}
		}

		spdlog::info("{} CodeCallback {} called", GetContextName(realContext), callback);
		if (!shouldCallCustomCallbacks)
			spdlog::info("Not executing custom callbacks for CodeCallback {}", callback);
		ret = ClientCallScriptInitCallback(sqvm, callback);

		// run after callbacks
		if (shouldCallCustomCallbacks)
		{
			for (Mod mod : g_ModManager->m_loadedMods)
			{
				if (!mod.Enabled)
					continue;

				for (ModScript script : mod.Scripts)
				{
					for (ModScriptCallback modCallback : script.Callbacks)
					{
						if (modCallback.Context == realContext && modCallback.AfterCallback.length())
						{
							spdlog::info(
								"Running custom {} script callback \"{}\"", GetContextName(realContext), modCallback.AfterCallback);
							ClientCallScriptInitCallback(sqvm, modCallback.AfterCallback.c_str());
						}
					}
				}
			}
		}
	}
	else if (context == ScriptContext::SERVER)
	{
		// since we don't hook arbitrary callbacks yet, make sure we're only doing callbacks on inits
		bool shouldCallCustomCallbacks = !strcmp(callback, "CodeCallback_MapSpawn");

		// run before callbacks
		// todo: we need to verify if RunOn is valid for current state before calling callbacks
		if (shouldCallCustomCallbacks)
		{
			for (Mod mod : g_ModManager->m_loadedMods)
			{
				if (!mod.Enabled)
					continue;

				for (ModScript script : mod.Scripts)
				{
					for (ModScriptCallback modCallback : script.Callbacks)
					{
						if (modCallback.Context == ScriptContext::SERVER && modCallback.BeforeCallback.length())
						{
							spdlog::info("Running custom {} script callback \"{}\"", GetContextName(context), modCallback.BeforeCallback);
							ServerCallScriptInitCallback(sqvm, modCallback.BeforeCallback.c_str());
						}
					}
				}
			}
		}

		spdlog::info("{} CodeCallback {} called", GetContextName(context), callback);
		if (!shouldCallCustomCallbacks)
			spdlog::info("Not executing custom callbacks for CodeCallback {}", callback);
		ret = ServerCallScriptInitCallback(sqvm, callback);

		// run after callbacks
		if (shouldCallCustomCallbacks)
		{
			for (Mod mod : g_ModManager->m_loadedMods)
			{
				if (!mod.Enabled)
					continue;

				for (ModScript script : mod.Scripts)
				{
					for (ModScriptCallback modCallback : script.Callbacks)
					{
						if (modCallback.Context == ScriptContext::SERVER && modCallback.AfterCallback.length())
						{
							spdlog::info("Running custom {} script callback \"{}\"", GetContextName(context), modCallback.AfterCallback);
							ServerCallScriptInitCallback(sqvm, modCallback.AfterCallback.c_str());
						}
					}
				}
			}
		}
	}

	return ret;
}

template <ScriptContext context> void ExecuteCodeCommand(const CCommand& args)
{
	if (context == ScriptContext::CLIENT)
		g_ClientSquirrelManager->ExecuteCode(args.ArgS());
	else if (context == ScriptContext::UI)
		g_UISquirrelManager->ExecuteCode(args.ArgS());
	else if (context == ScriptContext::SERVER)
		g_ServerSquirrelManager->ExecuteCode(args.ArgS());
}