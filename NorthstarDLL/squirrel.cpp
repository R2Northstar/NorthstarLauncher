#include "pch.h"
#include "squirrel.h"
#include "concommand.h"
#include "modmanager.h"
#include "r2engine.h"
#include "NSMem.h"

AUTOHOOK_INIT()

// inits
SquirrelManager<ScriptContext::CLIENT>* g_pClientSquirrel;
SquirrelManager<ScriptContext::SERVER>* g_pServerSquirrel;
SquirrelManager<ScriptContext::UI>* g_pUISquirrel;

template <ScriptContext context> SquirrelManager<context>* GetSquirrelManager() 
{
	switch (context)
	{
	case ScriptContext::CLIENT:
		return g_pClientSquirrel;
	case ScriptContext::SERVER:
		return g_pServerSquirrel;
	case ScriptContext::UI:
		return g_pUISquirrel;
	}
}

const char* GetContextName(ScriptContext context)
{
	switch (context)
	{
	case ScriptContext::CLIENT:
		return "CLIENT";
	case ScriptContext::SERVER:
		return "SERVER";
	case ScriptContext::UI:
		return "UI";
	}
}

// hooks
SQInteger (*ClientSQPrint)(void* sqvm, const char* fmt);
SQInteger (*UISQPrint)(void* sqvm, const char* fmt);
SQInteger (*ServerSQPrint)(void* sqvm, const char* fmt);
template <ScriptContext context> SQInteger SQPrintHook(void* sqvm, const char* fmt, ...)
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

void* (*ClientCreateNewVM)(void* a1, ScriptContext contextArg);
void* (*ServerCreateNewVM)(void* a1, ScriptContext contextArg);
template <ScriptContext context> void* CreateNewVMHook(void* a1, ScriptContext realContext)
{
	void* sqvm;

	if (context == ScriptContext::CLIENT)
	{
		sqvm = ClientCreateNewVM(a1, realContext);

		if (realContext == ScriptContext::UI)
			g_pUISquirrel->VMCreated(sqvm);
		else
			g_pClientSquirrel->VMCreated(sqvm);
	}
	else if (context == ScriptContext::SERVER)
	{
		sqvm = ServerCreateNewVM(a1, context);
		g_pServerSquirrel->VMCreated(sqvm);
	}

	spdlog::info("CreateNewVM {} {}", GetContextName(realContext), sqvm);
	return sqvm;
}

void (*ClientDestroyVM)(void* a1, void* sqvm);
void (*ServerDestroyVM)(void* a1, void* sqvm);
template <ScriptContext context> void DestroyVMHook(void* a1, void* sqvm)
{
	ScriptContext realContext = context; // ui and client use the same function so we use this for prints

	if (context == ScriptContext::CLIENT)
	{
		if (g_pClientSquirrel->sqvm == sqvm)
			g_pClientSquirrel->VMDestroyed();
		else if (g_pUISquirrel->sqvm == sqvm)
		{
			g_pUISquirrel->VMDestroyed();
			realContext = ScriptContext::UI;
		}

		ClientDestroyVM(a1, sqvm);
	}
	else if (context == ScriptContext::SERVER)
	{
		g_pServerSquirrel->VMDestroyed();
		ServerDestroyVM(a1, sqvm);
	}

	spdlog::info("DestroyVM {} {}", GetContextName(realContext), sqvm);
}

void (*ClientSQCompileError)(void* sqvm, const char* error, const char* file, int line, int column);
void (*ServerSQCompileError)(void* sqvm, const char* error, const char* file, int line, int column);
template <ScriptContext context> void ScriptCompileErrorHook(void* sqvm, const char* error, const char* file, int line, int column)
{
	ScriptContext realContext = context; // ui and client use the same function so we use this for prints
	if (context == ScriptContext::CLIENT && sqvm == g_pUISquirrel->sqvm2)
		realContext = ScriptContext::UI;

	spdlog::error("{} SCRIPT COMPILE ERROR {}", GetContextName(realContext), error);
	spdlog::error("{} line [{}] column [{}]", file, line, column);

	// use disconnect to display an error message for the compile error, but only if we aren't compiling from console, or from compilestring()
	// TODO: compilestring can actually define a custom buffer name as the second arg, we don't currently have a way of checking this
	// ideally we'd just check if the sqvm was fully initialised here, somehow
	if (strcmp(file, "console") && strcmp(file, "unnamedbuffer"))
	{
		R2::Cbuf_AddText(
			R2::Cbuf_GetCurrentPlayer(),
			fmt::format("disconnect \"Encountered {} script compilation error, see console for details.\"", GetContextName(realContext))
				.c_str(),
			R2::cmd_source_t::kCommandSrcCode);

		if (realContext == ScriptContext::UI) // likely temp: show console so user can see any errors, as error message wont display if ui is dead
			R2::Cbuf_AddText(R2::Cbuf_GetCurrentPlayer(), "showconsole", R2::cmd_source_t::kCommandSrcCode); 
	}

	// dont call the original function since it kills game lol
}

typedef bool (*CallScriptInitCallbackType)(void* sqvm, const char* callback);
CallScriptInitCallbackType ClientCallScriptInitCallback;
CallScriptInitCallbackType ServerCallScriptInitCallback;
template <ScriptContext context> bool CallScriptInitCallbackHook(void* sqvm, const char* callback)
{
	CallScriptInitCallbackType callScriptInitCallback;
	ScriptContext realContext = context;
	bool shouldCallCustomCallbacks = true;

	if (context == ScriptContext::CLIENT)
	{
		callScriptInitCallback = ClientCallScriptInitCallback;

		if (!strcmp(callback, "UICodeCallback_UIInit"))
			realContext = ScriptContext::UI;
		else if (strcmp(callback, "ClientCodeCallback_MapSpawn"))
			shouldCallCustomCallbacks = false;
	}
	else if (context == ScriptContext::SERVER)
	{
		callScriptInitCallback = ServerCallScriptInitCallback;
		shouldCallCustomCallbacks = !strcmp(callback, "CodeCallback_MapSpawn");
	}

	if (shouldCallCustomCallbacks)
	{
		for (Mod mod : g_pModManager->m_LoadedMods)
		{
			if (!mod.m_bEnabled)
				continue;

			for (ModScript script : mod.Scripts)
			{
				for (ModScriptCallback modCallback : script.Callbacks)
				{
					if (modCallback.Context == realContext && modCallback.BeforeCallback.length())
					{
						spdlog::info("Running custom {} script callback \"{}\"", GetContextName(realContext), modCallback.BeforeCallback);
						callScriptInitCallback(sqvm, modCallback.BeforeCallback.c_str());
					}
				}
			}
		}
	}

	spdlog::info("{} CodeCallback {} called", GetContextName(realContext), callback);
	if (!shouldCallCustomCallbacks)
		spdlog::info("Not executing custom callbacks for CodeCallback {}", callback);
	bool ret = callScriptInitCallback(sqvm, callback);

	// run after callbacks
	if (shouldCallCustomCallbacks)
	{
		for (Mod mod : g_pModManager->m_LoadedMods)
		{
			if (!mod.m_bEnabled)
				continue;

			for (ModScript script : mod.Scripts)
			{
				for (ModScriptCallback modCallback : script.Callbacks)
				{
					if (modCallback.Context == realContext && modCallback.AfterCallback.length())
					{
						spdlog::info("Running custom {} script callback \"{}\"", GetContextName(realContext), modCallback.AfterCallback);
						callScriptInitCallback(sqvm, modCallback.AfterCallback.c_str());
					}
				}
			}
		}
	}

	return ret;
}

template <ScriptContext context> void ConCommand_script(const CCommand& args)
{
	if (context == ScriptContext::CLIENT)
		g_pClientSquirrel->ExecuteCode(args.ArgS());
	else if (context == ScriptContext::UI)
		g_pUISquirrel->ExecuteCode(args.ArgS());
	else if (context == ScriptContext::SERVER)
		g_pServerSquirrel->ExecuteCode(args.ArgS());
}

ON_DLL_LOAD_RELIESON("client.dll", ClientSquirrel, ConCommand, (HMODULE baseAddress))
{
	AUTOHOOK_DISPATCH_MODULE(client.dll)

	g_pClientSquirrel = new SquirrelManager<ScriptContext::CLIENT>;
	g_pUISquirrel = new SquirrelManager<ScriptContext::UI>;
	
	g_pClientSquirrel->RegisterSquirrelFunc = (RegisterSquirrelFuncType)((char*)baseAddress + 0x108E0);
	g_pUISquirrel->RegisterSquirrelFunc = (RegisterSquirrelFuncType)((char*)baseAddress + 0x108E0);

	g_pClientSquirrel->__sq_compilebuffer = (sq_compilebufferType)((char*)baseAddress + 0x3110);
	g_pUISquirrel->__sq_compilebuffer = (sq_compilebufferType)((char*)baseAddress + 0x3110);
	g_pClientSquirrel->__sq_pushroottable = (sq_pushroottableType)((char*)baseAddress + 0x5860);
	g_pUISquirrel->__sq_pushroottable = (sq_pushroottableType)((char*)baseAddress + 0x5860);

	g_pClientSquirrel->__sq_call = (sq_callType)((char*)baseAddress + 0x8650);
	g_pUISquirrel->__sq_call = (sq_callType)((char*)baseAddress + 0x8650);

	g_pClientSquirrel->__sq_newarray = (sq_newarrayType)((char*)baseAddress + 0x39F0);
	g_pUISquirrel->__sq_newarray = (sq_newarrayType)((char*)baseAddress + 0x39F0);
	g_pClientSquirrel->__sq_arrayappend = (sq_arrayappendType)((char*)baseAddress + 0x3C70);
	g_pUISquirrel->__sq_arrayappend = (sq_arrayappendType)((char*)baseAddress + 0x3C70);

	g_pClientSquirrel->__sq_pushstring = (sq_pushstringType)((char*)baseAddress + 0x3440);
	g_pUISquirrel->__sq_pushstring = (sq_pushstringType)((char*)baseAddress + 0x3440);
	g_pClientSquirrel->__sq_pushinteger = (sq_pushintegerType)((char*)baseAddress + 0x36A0);
	g_pUISquirrel->__sq_pushinteger = (sq_pushintegerType)((char*)baseAddress + 0x36A0);
	g_pClientSquirrel->__sq_pushfloat = (sq_pushfloatType)((char*)baseAddress + 0x3800);
	g_pUISquirrel->__sq_pushfloat = (sq_pushfloatType)((char*)baseAddress + 0x3800);
	g_pClientSquirrel->__sq_pushbool = (sq_pushboolType)((char*)baseAddress + 0x3710);
	g_pUISquirrel->__sq_pushbool = (sq_pushboolType)((char*)baseAddress + 0x3710);
	g_pClientSquirrel->__sq_raiseerror = (sq_raiseerrorType)((char*)baseAddress + 0x8470);
	g_pUISquirrel->__sq_raiseerror = (sq_raiseerrorType)((char*)baseAddress + 0x8470);

	g_pClientSquirrel->__sq_getstring = (sq_getstringType)((char*)baseAddress + 0x60C0);
	g_pUISquirrel->__sq_getstring = (sq_getstringType)((char*)baseAddress + 0x60C0);
	g_pClientSquirrel->__sq_getinteger = (sq_getintegerType)((char*)baseAddress + 0x60E0);
	g_pUISquirrel->__sq_getinteger = (sq_getintegerType)((char*)baseAddress + 0x60E0);
	g_pClientSquirrel->__sq_getfloat = (sq_getfloatType)((char*)baseAddress + 0x6100);
	g_pUISquirrel->__sq_getfloat = (sq_getfloatType)((char*)baseAddress + 0x6100);
	g_pClientSquirrel->__sq_getbool = (sq_getboolType)((char*)baseAddress + 0x6130);
	g_pUISquirrel->__sq_getbool = (sq_getboolType)((char*)baseAddress + 0x6130);
	g_pClientSquirrel->__sq_get = (sq_getType)((char*)baseAddress + 0x7C30);
	g_pUISquirrel->__sq_get = (sq_getType)((char*)baseAddress + 0x7C30);

	// uiscript_reset concommand: don't loop forever if compilation fails
	NSMem::NOP((uintptr_t)baseAddress + 0x3C6E4C, 6);

	MAKEHOOK((char*)baseAddress + 0x12B00, &SQPrintHook<ScriptContext::CLIENT>, &ClientSQPrint); // client print function
	MAKEHOOK((char*)baseAddress + 0x12BA0, &SQPrintHook<ScriptContext::UI>, &UISQPrint); // ui print function

	MAKEHOOK((char*)baseAddress + 0x26130, &CreateNewVMHook<ScriptContext::CLIENT>, &ClientCreateNewVM); // client createnewvm function
	MAKEHOOK((char*)baseAddress + 0x26E70, &DestroyVMHook<ScriptContext::CLIENT>, &ClientDestroyVM); // client destroyvm function
	MAKEHOOK(
		(char*)baseAddress + 0x79A50,
		&ScriptCompileErrorHook<ScriptContext::CLIENT>,
		&ClientSQCompileError); // client compileerror function
	MAKEHOOK(
		(char*)baseAddress + 0x10190,
		&CallScriptInitCallbackHook<ScriptContext::CLIENT>,
		&ClientCallScriptInitCallback); // client callscriptinitcallback function

	RegisterConCommand("script_client", ConCommand_script<ScriptContext::CLIENT>, "Executes script code on the client vm", FCVAR_CLIENTDLL);
	RegisterConCommand("script_ui", ConCommand_script<ScriptContext::UI>, "Executes script code on the ui vm", FCVAR_CLIENTDLL);
}

ON_DLL_LOAD_RELIESON("server.dll", ServerSquirrel, ConCommand, (HMODULE baseAddress))
{
	AUTOHOOK_DISPATCH_MODULE(server.dll)

	g_pServerSquirrel = new SquirrelManager<ScriptContext::SERVER>;

	g_pServerSquirrel->RegisterSquirrelFunc = (RegisterSquirrelFuncType)((char*)baseAddress + 0x1DD10);

	g_pServerSquirrel->__sq_compilebuffer = (sq_compilebufferType)((char*)baseAddress + 0x3110);
	g_pServerSquirrel->__sq_pushroottable = (sq_pushroottableType)((char*)baseAddress + 0x5840);
	g_pServerSquirrel->__sq_call = (sq_callType)((char*)baseAddress + 0x8620);

	g_pServerSquirrel->__sq_newarray = (sq_newarrayType)((char*)baseAddress + 0x39F0);
	g_pServerSquirrel->__sq_arrayappend = (sq_arrayappendType)((char*)baseAddress + 0x3C70);

	g_pServerSquirrel->__sq_pushstring = (sq_pushstringType)((char*)baseAddress + 0x3440);
	g_pServerSquirrel->__sq_pushinteger = (sq_pushintegerType)((char*)baseAddress + 0x36A0);
	g_pServerSquirrel->__sq_pushfloat = (sq_pushfloatType)((char*)baseAddress + 0x3800);
	g_pServerSquirrel->__sq_pushbool = (sq_pushboolType)((char*)baseAddress + 0x3710);
	g_pServerSquirrel->__sq_raiseerror = (sq_raiseerrorType)((char*)baseAddress + 0x8440);

	g_pServerSquirrel->__sq_getstring = (sq_getstringType)((char*)baseAddress + 0x60A0);
	g_pServerSquirrel->__sq_getinteger = (sq_getintegerType)((char*)baseAddress + 0x60C0);
	g_pServerSquirrel->__sq_getfloat = (sq_getfloatType)((char*)baseAddress + 0x60E0);
	g_pServerSquirrel->__sq_getbool = (sq_getboolType)((char*)baseAddress + 0x6110);
	g_pServerSquirrel->__sq_get = (sq_getType)((char*)baseAddress + 0x7C00);

	MAKEHOOK((char*)baseAddress + 0x1FE90, &SQPrintHook<ScriptContext::SERVER>, &ServerSQPrint); // server print function
	MAKEHOOK((char*)baseAddress + 0x260E0, &CreateNewVMHook<ScriptContext::SERVER>, &ServerCreateNewVM); // server createnewvm function
	MAKEHOOK((char*)baseAddress + 0x26E20, &DestroyVMHook<ScriptContext::SERVER>, &ServerDestroyVM); // server destroyvm function
	MAKEHOOK(
		(char*)baseAddress + 0x799E0,
		&ScriptCompileErrorHook<ScriptContext::SERVER>,
		&ServerSQCompileError); // server compileerror function
	MAKEHOOK(
		(char*)baseAddress + 0x1D5C0,
		&CallScriptInitCallbackHook<ScriptContext::SERVER>,
		&ServerCallScriptInitCallback); // server callscriptinitcallback function

	// FCVAR_CHEAT and FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS allows clients to execute this, but since it's unsafe we only allow it when cheats are enabled
	// for script_client and script_ui, we don't use cheats, so clients can execute them on themselves all they want
	RegisterConCommand(
		"script",
		ConCommand_script<ScriptContext::SERVER>,
		"Executes script code on the server vm",
		FCVAR_GAMEDLL | FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS | FCVAR_CHEAT);
}