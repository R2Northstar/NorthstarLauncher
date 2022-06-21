#include "pch.h"
#include "squirrel.h"
#include "concommand.h"
#include "modmanager.h"
#include "r2engine.h"
#include "NSMem.h"

RegisterSquirrelFuncType ClientRegisterSquirrelFunc;
RegisterSquirrelFuncType ServerRegisterSquirrelFunc;

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
typedef SQInteger (*SQPrintType)(void* sqvm, char* fmt, ...);
SQPrintType ClientSQPrint;
SQPrintType UISQPrint;
SQPrintType ServerSQPrint;
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

typedef void* (*CreateNewVMType)(void* a1, ScriptContext contextArg);
CreateNewVMType ClientCreateNewVM;
CreateNewVMType ServerCreateNewVM;
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

typedef void (*DestroyVMType)(void* a1, void* sqvm);
DestroyVMType ClientDestroyVM;
DestroyVMType ServerDestroyVM;
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

typedef void (*ScriptCompileError)(void* sqvm, const char* error, const char* file, int line, int column);
ScriptCompileError ClientSQCompileError;
ScriptCompileError ServerSQCompileError;
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

		if (realContext == ScriptContext::UI) // likely temp: show console so user can see any errors
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
		for (Mod mod : g_pModManager->m_loadedMods)
		{
			if (!mod.Enabled)
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
		for (Mod mod : g_pModManager->m_loadedMods)
		{
			if (!mod.Enabled)
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

ON_DLL_LOAD_RELIESON("client.dll", ClientSquirrel, ConCommand, [](HMODULE baseAddress)
{
	HookEnabler hook;

	// client inits
	g_pClientSquirrel = new SquirrelManager<ScriptContext::CLIENT>;

	ENABLER_CREATEHOOK(
		hook,
		(char*)baseAddress + 0x12B00,
		&SQPrintHook<ScriptContext::CLIENT>,
		reinterpret_cast<LPVOID*>(&ClientSQPrint)); // client print function
	RegisterConCommand("script_client", ConCommand_script<ScriptContext::CLIENT>, "Executes script code on the client vm", FCVAR_CLIENTDLL);

	// ui inits
	g_pUISquirrel = new SquirrelManager<ScriptContext::UI>;

	ENABLER_CREATEHOOK(
		hook,
		(char*)baseAddress + 0x12BA0,
		&SQPrintHook<ScriptContext::UI>,
		reinterpret_cast<LPVOID*>(&UISQPrint)); // ui print function
	RegisterConCommand("script_ui", ConCommand_script<ScriptContext::UI>, "Executes script code on the ui vm", FCVAR_CLIENTDLL);
	
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

ON_DLL_LOAD_RELIESON("server.dll", ServerSquirrel, ConCommand, [](HMODULE baseAddress)
{
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
		ConCommand_script<ScriptContext::SERVER>,
		"Executes script code on the server vm",
		FCVAR_GAMEDLL | FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS | FCVAR_CHEAT);
})

int GetReturnTypeEnumFromString(const char* name) {
	

	static std::map<std::string, SQReturnTypeEnum> sqEnumStrMap = {
		{"bool",	SqReturnBoolean },
		{"float",	SqReturnFloat },
		{"vector",	SqReturnVector },
		{"int",		SqReturnInteger },
		{"entity",	SqReturnEntity },
		{"string",	SqReturnString },
		{"array",	SqReturnArrays },
		{"asset",	SqReturnAsset },
		{"table",	SqReturnTable }
	};

	if (sqEnumStrMap.count(name)) {
		return sqEnumStrMap[name];
	}
	else {
		return SqReturnStringOrNull; //previous default value
	}
}