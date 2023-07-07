#include "squirrel.h"
#include "logging/logging.h"
#include "core/convar/concommand.h"
#include "mods/modmanager.h"
#include "dedicated/dedicated.h"
#include "engine/r2engine.h"
#include "core/tier0.h"
#include "plugins/plugin_abi.h"
#include "plugins/plugins.h"

#include <any>

AUTOHOOK_INIT()

std::shared_ptr<ColoredLogger> getSquirrelLoggerByContext(ScriptContext context)
{
	switch (context)
	{
	case ScriptContext::UI:
		return NS::log::SCRIPT_UI;
	case ScriptContext::CLIENT:
		return NS::log::SCRIPT_CL;
	case ScriptContext::SERVER:
		return NS::log::SCRIPT_SV;
	default:
		throw std::runtime_error("getSquirrelLoggerByContext called with invalid context");
		return nullptr;
	}
}

namespace NS::log
{
	template <ScriptContext context> std::shared_ptr<spdlog::logger> squirrel_logger()
	{
		// Switch statements can't be constexpr afaik
		// clang-format off
		if constexpr (context == ScriptContext::UI) { return SCRIPT_UI; }
		if constexpr (context == ScriptContext::CLIENT) { return SCRIPT_CL; }
		if constexpr (context == ScriptContext::SERVER) { return SCRIPT_SV; }
		// clang-format on
	}
}; // namespace NS::log

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
	default:
		return "UNKNOWN";
	}
}

const char* GetContextName_Short(ScriptContext context)
{
	switch (context)
	{
	case ScriptContext::CLIENT:
		return "CL";
	case ScriptContext::SERVER:
		return "SV";
	case ScriptContext::UI:
		return "UI";
	default:
		return "??";
	}
}

eSQReturnType SQReturnTypeFromString(const char* pReturnType)
{
	static const std::map<std::string, eSQReturnType> sqReturnTypeNameToString = {
		{"bool", eSQReturnType::Boolean},
		{"float", eSQReturnType::Float},
		{"vector", eSQReturnType::Vector},
		{"int", eSQReturnType::Integer},
		{"entity", eSQReturnType::Entity},
		{"string", eSQReturnType::String},
		{"array", eSQReturnType::Arrays},
		{"asset", eSQReturnType::Asset},
		{"table", eSQReturnType::Table}};

	if (sqReturnTypeNameToString.find(pReturnType) != sqReturnTypeNameToString.end())
		return sqReturnTypeNameToString.at(pReturnType);
	else
		return eSQReturnType::Default; // previous default value
}

ScriptContext ScriptContextFromString(std::string string)
{
	if (string == "UI")
		return ScriptContext::UI;
	if (string == "CLIENT")
		return ScriptContext::CLIENT;
	if (string == "SERVER")
		return ScriptContext::SERVER;
	else
		return ScriptContext::INVALID;
}

const char* SQTypeNameFromID(int type)
{
	switch (type)
	{
	case OT_ASSET:
		return "asset";
	case OT_INTEGER:
		return "int";
	case OT_BOOL:
		return "bool";
	case SQOBJECT_NUMERIC:
		return "float or int";
	case OT_NULL:
		return "null";
	case OT_VECTOR:
		return "vector";
	case 0:
		return "var";
	case OT_USERDATA:
		return "userdata";
	case OT_FLOAT:
		return "float";
	case OT_STRING:
		return "string";
	case OT_ARRAY:
		return "array";
	case 0x8000200:
		return "function";
	case 0x8100000:
		return "structdef";
	case OT_THREAD:
		return "thread";
	case OT_FUNCPROTO:
		return "function";
	case OT_CLAAS:
		return "class";
	case OT_WEAKREF:
		return "weakref";
	case 0x8080000:
		return "unimplemented function";
	case 0x8200000:
		return "struct instance";
	case OT_TABLE:
		return "table";
	case 0xA008000:
		return "instance";
	case OT_ENTITY:
		return "entity";
	}
	return "";
}

template <ScriptContext context> void SquirrelManager<context>::GenerateSquirrelFunctionsStruct(SquirrelFunctions* s)
{
	s->RegisterSquirrelFunc = RegisterSquirrelFunc;
	s->__sq_defconst = __sq_defconst;

	s->__sq_compilebuffer = __sq_compilebuffer;
	s->__sq_call = __sq_call;
	s->__sq_raiseerror = __sq_raiseerror;

	s->__sq_newarray = __sq_newarray;
	s->__sq_arrayappend = __sq_arrayappend;

	s->__sq_newtable = __sq_newtable;
	s->__sq_newslot = __sq_newslot;

	s->__sq_pushroottable = __sq_pushroottable;
	s->__sq_pushstring = __sq_pushstring;
	s->__sq_pushinteger = __sq_pushinteger;
	s->__sq_pushfloat = __sq_pushfloat;
	s->__sq_pushbool = __sq_pushbool;
	s->__sq_pushasset = __sq_pushasset;
	s->__sq_pushvector = __sq_pushvector;
	s->__sq_pushobject = __sq_pushobject;
	s->__sq_getstring = __sq_getstring;
	s->__sq_getthisentity = __sq_getthisentity;
	s->__sq_getobject = __sq_getobject;

	s->__sq_stackinfos = __sq_stackinfos;

	s->__sq_getinteger = __sq_getinteger;
	s->__sq_getfloat = __sq_getfloat;
	s->__sq_getbool = __sq_getbool;
	s->__sq_get = __sq_get;
	s->__sq_getasset = __sq_getasset;
	s->__sq_getuserdata = __sq_getuserdata;
	s->__sq_getvector = __sq_getvector;
	s->__sq_createuserdata = __sq_createuserdata;
	s->__sq_setuserdatatypeid = __sq_setuserdatatypeid;
	s->__sq_getfunction = __sq_getfunction;

	s->__sq_getentityfrominstance = __sq_getentityfrominstance;
	s->__sq_GetEntityConstant_CBaseEntity = __sq_GetEntityConstant_CBaseEntity;

	s->__sq_schedule_call_external = AsyncCall_External;
}

// Allows for generating squirrelmessages from plugins.
// Not used in this version, but will be used later
void AsyncCall_External(ScriptContext context, const char* func_name, SquirrelMessage_External_Pop function)
{
	SquirrelMessage message {};
	message.functionName = func_name;
	message.isExternal = true;
	message.externalFunc = function;
	switch (context)
	{
	case ScriptContext::CLIENT:
		g_pSquirrel<ScriptContext::CLIENT>->messageBuffer->push(message);
		break;
	case ScriptContext::SERVER:
		g_pSquirrel<ScriptContext::SERVER>->messageBuffer->push(message);
		break;
	case ScriptContext::UI:
		g_pSquirrel<ScriptContext::UI>->messageBuffer->push(message);
		break;
	}
}

// needed to define implementations for squirrelmanager outside of squirrel.h without compiler errors
template class SquirrelManager<ScriptContext::SERVER>;
template class SquirrelManager<ScriptContext::CLIENT>;
template class SquirrelManager<ScriptContext::UI>;

template <ScriptContext context> void SquirrelManager<context>::VMCreated(CSquirrelVM* newSqvm)
{
	m_pSQVM = newSqvm;

	for (SQFuncRegistration* funcReg : m_funcRegistrations)
	{
		spdlog::info("Registering {} function {}", GetContextName(context), funcReg->squirrelFuncName);
		RegisterSquirrelFunc(m_pSQVM, funcReg, 1);
	}

	for (auto& pair : g_pModManager->m_DependencyConstants)
	{
		bool bWasFound = false;

		for (Mod& dependency : g_pModManager->m_LoadedMods)
		{
			if (!dependency.m_bEnabled)
				continue;

			if (dependency.Name == pair.second)
			{
				bWasFound = true;
				break;
			}
		}

		defconst(m_pSQVM, pair.first.c_str(), bWasFound);
	}
	g_pSquirrel<context>->messageBuffer = new SquirrelMessageBuffer();
	g_pPluginManager->InformSQVMCreated(context, newSqvm);
}

template <ScriptContext context> void SquirrelManager<context>::VMDestroyed()
{
	// Call all registered mod Destroy callbacks.
	if (g_pModManager)
	{
		NS::log::squirrel_logger<context>()->info("Calling Destroy callbacks for all loaded mods.");

		for (const Mod& loadedMod : g_pModManager->m_LoadedMods)
		{
			for (const ModScript& script : loadedMod.Scripts)
			{
				for (const ModScriptCallback& callback : script.Callbacks)
				{
					if (callback.Context != context || callback.DestroyCallback.length() == 0)
					{
						continue;
					}

					Call(callback.DestroyCallback.c_str());
					NS::log::squirrel_logger<context>()->info("Executed Destroy callback {}.", callback.DestroyCallback);
				}
			}
		}
	}

	g_pPluginManager->InformSQVMDestroyed(context);

	// Discard the previous vm and delete the message buffer.
	m_pSQVM = nullptr;

	delete g_pSquirrel<context>->messageBuffer;
	g_pSquirrel<context>->messageBuffer = nullptr;
}

template <ScriptContext context> void SquirrelManager<context>::ExecuteCode(const char* pCode)
{
	if (!m_pSQVM || !m_pSQVM->sqvm)
	{
		spdlog::error("Cannot execute code, {} squirrel vm is not initialised", GetContextName(context));
		return;
	}

	spdlog::info("Executing {} script code {} ", GetContextName(context), pCode);

	std::string strCode(pCode);
	CompileBufferState bufferState = CompileBufferState(strCode);

	SQRESULT compileResult = compilebuffer(&bufferState, "console");
	spdlog::info("sq_compilebuffer returned {}", PrintSQRESULT.at(compileResult));

	if (compileResult != SQRESULT_ERROR)
	{
		pushroottable(m_pSQVM->sqvm);
		SQRESULT callResult = _call(m_pSQVM->sqvm, 0);
		spdlog::info("sq_call returned {}", PrintSQRESULT.at(callResult));
	}
}

template <ScriptContext context> void SquirrelManager<context>::AddFuncRegistration(
	std::string returnType, std::string name, std::string argTypes, std::string helpText, SQFunction func)
{
	SQFuncRegistration* reg = new SQFuncRegistration;

	reg->squirrelFuncName = new char[name.size() + 1];
	strcpy((char*)reg->squirrelFuncName, name.c_str());
	reg->cppFuncName = reg->squirrelFuncName;

	reg->helpText = new char[helpText.size() + 1];
	strcpy((char*)reg->helpText, helpText.c_str());

	reg->returnTypeString = new char[returnType.size() + 1];
	strcpy((char*)reg->returnTypeString, returnType.c_str());
	reg->returnType = SQReturnTypeFromString(returnType.c_str());

	reg->argTypes = new char[argTypes.size() + 1];
	strcpy((char*)reg->argTypes, argTypes.c_str());

	reg->funcPtr = func;

	m_funcRegistrations.push_back(reg);
}

template <ScriptContext context> SQRESULT SquirrelManager<context>::setupfunc(const SQChar* funcname)
{
	pushroottable(m_pSQVM->sqvm);
	pushstring(m_pSQVM->sqvm, funcname, -1);

	SQRESULT result = get(m_pSQVM->sqvm, -2);
	if (result != SQRESULT_ERROR)
		pushroottable(m_pSQVM->sqvm);

	return result;
}

template <ScriptContext context> void SquirrelManager<context>::AddFuncOverride(std::string name, SQFunction func)
{
	m_funcOverrides[name] = func;
}

// hooks
bool IsUIVM(ScriptContext context, HSquirrelVM* pSqvm)
{
	return ScriptContext(pSqvm->sharedState->cSquirrelVM->vmContext) == ScriptContext::UI;
}

template <ScriptContext context> void* (*__fastcall sq_compiler_create)(HSquirrelVM* sqvm, void* a2, void* a3, SQBool bShouldThrowError);
template <ScriptContext context> void* __fastcall sq_compiler_createHook(HSquirrelVM* sqvm, void* a2, void* a3, SQBool bShouldThrowError)
{
	// store whether errors generated from this compile should be fatal
	if (IsUIVM(context, sqvm))
		g_pSquirrel<ScriptContext::UI>->m_bFatalCompilationErrors = bShouldThrowError;
	else
		g_pSquirrel<context>->m_bFatalCompilationErrors = bShouldThrowError;

	return sq_compiler_create<context>(sqvm, a2, a3, bShouldThrowError);
}

template <ScriptContext context> SQInteger (*SQPrint)(HSquirrelVM* sqvm, const char* fmt);
template <ScriptContext context> SQInteger SQPrintHook(HSquirrelVM* sqvm, const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);

	SQChar buf[1024];
	int charsWritten = vsnprintf_s(buf, _TRUNCATE, fmt, va);

	if (charsWritten > 0)
	{
		if (buf[charsWritten - 1] == '\n')
			buf[charsWritten - 1] = '\0';
		g_pSquirrel<context>->logger->info("{}", buf);
	}

	va_end(va);
	return 0;
}

template <ScriptContext context> CSquirrelVM* (*__fastcall CreateNewVM)(void* a1, ScriptContext realContext);
template <ScriptContext context> CSquirrelVM* __fastcall CreateNewVMHook(void* a1, ScriptContext realContext)
{
	CSquirrelVM* sqvm = CreateNewVM<context>(a1, realContext);
	if (realContext == ScriptContext::UI)
		g_pSquirrel<ScriptContext::UI>->VMCreated(sqvm);
	else
		g_pSquirrel<context>->VMCreated(sqvm);

	spdlog::info("CreateNewVM {} {}", GetContextName(realContext), (void*)sqvm);
	return sqvm;
}

template <ScriptContext context> bool (*__fastcall CSquirrelVM_init)(CSquirrelVM* vm, ScriptContext realContext, float time);
template <ScriptContext context> bool __fastcall CSquirrelVM_initHook(CSquirrelVM* vm, ScriptContext realContext, float time)
{
	bool ret = CSquirrelVM_init<context>(vm, realContext, time);
	for (Mod mod : g_pModManager->m_LoadedMods)
	{
		if (mod.m_bEnabled && mod.initScript.size() != 0)
		{
			std::string name = mod.initScript.substr(mod.initScript.find_last_of('/') + 1);
			std::string path = std::string("scripts/vscripts/") + mod.initScript;
			if (g_pSquirrel<context>->compilefile(vm, path.c_str(), name.c_str(), 0))
				g_pSquirrel<context>->compilefile(vm, path.c_str(), name.c_str(), 1);
		}
	}
	return ret;
}

template <ScriptContext context> void (*__fastcall DestroyVM)(void* a1, CSquirrelVM* sqvm);
template <ScriptContext context> void __fastcall DestroyVMHook(void* a1, CSquirrelVM* sqvm)
{
	ScriptContext realContext = context; // ui and client use the same function so we use this for prints
	if (IsUIVM(context, sqvm->sqvm))
	{
		realContext = ScriptContext::UI;
		g_pSquirrel<ScriptContext::UI>->VMDestroyed();
		DestroyVM<ScriptContext::CLIENT>(a1, sqvm); // If we pass UI here it crashes
	}
	else
	{
		g_pSquirrel<context>->VMDestroyed();
		DestroyVM<context>(a1, sqvm);
	}

	spdlog::info("DestroyVM {} {}", GetContextName(realContext), (void*)sqvm);
}

template <ScriptContext context>
void (*__fastcall SQCompileError)(HSquirrelVM* sqvm, const char* error, const char* file, int line, int column);
template <ScriptContext context>
void __fastcall ScriptCompileErrorHook(HSquirrelVM* sqvm, const char* error, const char* file, int line, int column)
{
	bool bIsFatalError = g_pSquirrel<context>->m_bFatalCompilationErrors;
	ScriptContext realContext = context; // ui and client use the same function so we use this for prints
	if (IsUIVM(context, sqvm))
	{
		realContext = ScriptContext::UI;
		bIsFatalError = g_pSquirrel<ScriptContext::UI>->m_bFatalCompilationErrors;
	}

	auto logger = getSquirrelLoggerByContext(realContext);

	logger->error("COMPILE ERROR {}", error);
	logger->error("{} line [{}] column [{}]", file, line, column);

	// use disconnect to display an error message for the compile error, but only if the compilation error was fatal
	// todo, we could get this from sqvm itself probably, rather than hooking sq_compiler_create
	if (bIsFatalError)
	{
		// kill dedicated server if we hit this
		if (IsDedicatedServer())
		{
			logger->error("Exiting dedicated server, compile error is fatal");
			// flush the logger before we exit so debug things get saved to log file
			logger->flush();
			exit(EXIT_FAILURE);
		}
		else
		{
			R2::Cbuf_AddText(
				R2::Cbuf_GetCurrentPlayer(),
				fmt::format("disconnect \"Encountered {} script compilation error, see console for details.\"", GetContextName(realContext))
					.c_str(),
				R2::cmd_source_t::kCommandSrcCode);

			// likely temp: show console so user can see any errors, as error message wont display if ui is dead
			// maybe we could disable all mods other than the coremods and try a reload before doing this?
			// could also maybe do some vgui bullshit to show something visually rather than console
			if (realContext == ScriptContext::UI)
				R2::Cbuf_AddText(R2::Cbuf_GetCurrentPlayer(), "showconsole", R2::cmd_source_t::kCommandSrcCode);
		}
	}

	// dont call the original function since it kills game lol
}

template <ScriptContext context>
int64_t (*__fastcall RegisterSquirrelFunction)(CSquirrelVM* sqvm, SQFuncRegistration* funcReg, char unknown);
template <ScriptContext context>
int64_t __fastcall RegisterSquirrelFunctionHook(CSquirrelVM* sqvm, SQFuncRegistration* funcReg, char unknown)
{
	if (IsUIVM(context, sqvm->sqvm))
	{
		if (g_pSquirrel<ScriptContext::UI>->m_funcOverrides.count(funcReg->squirrelFuncName))
		{
			g_pSquirrel<ScriptContext::UI>->m_funcOriginals[funcReg->squirrelFuncName] = funcReg->funcPtr;
			funcReg->funcPtr = g_pSquirrel<ScriptContext::UI>->m_funcOverrides[funcReg->squirrelFuncName];
			spdlog::info("Replacing {} in UI", std::string(funcReg->squirrelFuncName));
		}

		return g_pSquirrel<ScriptContext::UI>->RegisterSquirrelFunc(sqvm, funcReg, unknown);
	}

	if (g_pSquirrel<context>->m_funcOverrides.find(funcReg->squirrelFuncName) != g_pSquirrel<context>->m_funcOverrides.end())
	{
		g_pSquirrel<context>->m_funcOriginals[funcReg->squirrelFuncName] = funcReg->funcPtr;
		funcReg->funcPtr = g_pSquirrel<context>->m_funcOverrides[funcReg->squirrelFuncName];
		spdlog::info("Replacing {} in Client", std::string(funcReg->squirrelFuncName));
	}

	return g_pSquirrel<context>->RegisterSquirrelFunc(sqvm, funcReg, unknown);
}

template <ScriptContext context> bool (*__fastcall CallScriptInitCallback)(void* sqvm, const char* callback);
template <ScriptContext context> bool __fastcall CallScriptInitCallbackHook(void* sqvm, const char* callback)
{
	ScriptContext realContext = context;
	bool bShouldCallCustomCallbacks = true;

	if (context == ScriptContext::CLIENT)
	{
		if (!strcmp(callback, "UICodeCallback_UIInit"))
			realContext = ScriptContext::UI;
		else if (strcmp(callback, "ClientCodeCallback_MapSpawn"))
			bShouldCallCustomCallbacks = false;
	}
	else if (context == ScriptContext::SERVER)
		bShouldCallCustomCallbacks = !strcmp(callback, "CodeCallback_MapSpawn");

	if (bShouldCallCustomCallbacks)
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
						CallScriptInitCallback<context>(sqvm, modCallback.BeforeCallback.c_str());
					}
				}
			}
		}
	}

	spdlog::info("{} CodeCallback {} called", GetContextName(realContext), callback);
	if (!bShouldCallCustomCallbacks)
		spdlog::info("Not executing custom callbacks for CodeCallback {}", callback);
	bool ret = CallScriptInitCallback<context>(sqvm, callback);

	// run after callbacks
	if (bShouldCallCustomCallbacks)
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
						CallScriptInitCallback<context>(sqvm, modCallback.AfterCallback.c_str());
					}
				}
			}
		}
	}

	return ret;
}

template <ScriptContext context> void ConCommand_script(const CCommand& args)
{
	g_pSquirrel<context>->ExecuteCode(args.ArgS());
}

// literal class type that wraps a constant expression string
template <size_t N> struct TemplateStringLiteral
{
	constexpr TemplateStringLiteral(const char (&str)[N])
	{
		std::copy_n(str, N, value);
	}

	char value[N];
};

template <ScriptContext context, TemplateStringLiteral funcName> SQRESULT SQ_StubbedFunc(HSquirrelVM* sqvm)
{
	spdlog::info("Blocking call to stubbed function {} in {}", funcName.value, GetContextName(context));
	return SQRESULT_NULL;
}

template <ScriptContext context> void StubUnsafeSQFuncs()
{
	if (!Tier0::CommandLine()->CheckParm("-allowunsafesqfuncs"))
	{
		g_pSquirrel<context>->AddFuncOverride("DevTextBufferWrite", SQ_StubbedFunc<context, "DevTextBufferWrite">);
		g_pSquirrel<context>->AddFuncOverride("DevTextBufferClear", SQ_StubbedFunc<context, "DevTextBufferClear">);
		g_pSquirrel<context>->AddFuncOverride("DevTextBufferDumpToFile", SQ_StubbedFunc<context, "DevTextBufferDumpToFile">);
		g_pSquirrel<context>->AddFuncOverride("Dev_CommandLineAddParam", SQ_StubbedFunc<context, "Dev_CommandLineAddParam">);
		g_pSquirrel<context>->AddFuncOverride("DevP4Checkout", SQ_StubbedFunc<context, "DevP4Checkout">);
		g_pSquirrel<context>->AddFuncOverride("DevP4Add", SQ_StubbedFunc<context, "DevP4Add">);
	}
}

template <ScriptContext context> void SquirrelManager<context>::ProcessMessageBuffer()
{
	while (std::optional<SquirrelMessage> maybeMessage = messageBuffer->pop())
	{
		SquirrelMessage message = maybeMessage.value();

		SQObject functionobj {};
		int result = sq_getfunction(m_pSQVM->sqvm, message.functionName.c_str(), &functionobj, 0);
		if (result != 0) // This func returns 0 on success for some reason
		{
			NS::log::squirrel_logger<context>()->error(
				"ProcessMessageBuffer was unable to find function with name '{}'. Is it global?", message.functionName);
			continue;
		}

		pushobject(m_pSQVM->sqvm, &functionobj); // Push the function object
		pushroottable(m_pSQVM->sqvm);

		if (message.isExternal)
		{
			message.externalFunc(m_pSQVM->sqvm);
		}
		else
		{
			for (auto& v : message.args)
			{
				// Execute lambda to push arg to stack
				v();
			}
		}

		_call(m_pSQVM->sqvm, message.args.size());
	}
}

/*
	Initialize a function on the CLIENT and UI or SERVER, depending on context
	The member __needs__ to be a member of the manager passed. Otherwise you will write to random memory!
	Undefined Behaviour ahead!
*/
template <ScriptContext context, typename T>
void SetSharedMember(SquirrelManager<context>* manager, T* member, const CModule module, const int clOffset, const int svOffset)
{
	switch (context)
	{
	case ScriptContext::CLIENT:
	case ScriptContext::UI:
	{
		/*
			The CLIENT & UI VM share the same squirrel functions in the client.dll.
			Because of this we only need an offset for the SERVER and the CLIENT.
			This branch sets the member for __both__ the CLIENT and UI SquirrelManager.
		*/

		// Initialize the member passed. It's assumed that the 'member' parameter is a member of the 'manager' parameter.
		*member = module.Offset(clOffset).As<T>();

		// If the context of this function is CLIENT, initialize the same member on the UI SquirrelManager,
		// otherwise initialize the member of the CLIENT manager
		*(T*)((char*)g_pSquirrel < context == ScriptContext::UI ? ScriptContext::CLIENT : ScriptContext::UI > +(int64_t)member - (int64_t)manager) =
			*member;
		/*
			The calculation for the member to set can be simplified like this:

			```
			int64_t managerAddress = (int64_t)manager; // Same size as a pointer
			int64_t memberAddress = (int64_t)member;
			int64_t memberOffset = memberAddress - managerAddress // This is the offset of the manager from the base address from the
		   manager class instance

			int64_t otherManagerMemberAddres = (int64_t)g_pSquirrel<otherContext> + memberOffset;
			T* otherManagerMember = (T*)otherManagerMemberAddress;
			```

			It basically only calculates the offset from the member of the class and adds it to the address of another SquirrelManager to
		   get the same member on a different instance.
		*/
		break;
	}
	case ScriptContext::SERVER:
		// The SERVER context is the only vm using these so we don't need to initialize another SquirrelManager
		*member = module.Offset(svOffset).As<T>();
	}
}

template <ScriptContext context> void InitSharedSquirrelFunctions(SquirrelManager<context>* manager, CModule module)
{
	// consts & root table
	SetSharedMember<context, sq_defconstType>(manager, &manager->__sq_defconst, module, 0x12120, 0x1F550);
	SetSharedMember<context, sq_pushroottableType>(manager, &manager->__sq_pushroottable, module, 0x5860, 0x5840);

	// compiler api
	SetSharedMember<context, sq_compilebufferType>(manager, &manager->__sq_compilebuffer, module, 0x3110, 0x3110);
	SetSharedMember<context, sq_compilefileType>(manager, &manager->__sq_compilefile, module, 0xF950, 0x1CD80);

	// calls
	SetSharedMember<context, sq_callType>(manager, &manager->__sq_call, module, 0x8650, 0x8620);
	SetSharedMember<context, sq_raiseerrorType>(manager, &manager->__sq_raiseerror, module, 0x8470, 0x8440);

	// arrays
	SetSharedMember<context, sq_newarrayType>(manager, &manager->__sq_newarray, module, 0x39F0, 0x39F0);
	SetSharedMember<context, sq_arrayappendType>(manager, &manager->__sq_arrayappend, module, 0x3C70, 0x3C70);

	// tables
	SetSharedMember<context, sq_newtableType>(manager, &manager->__sq_newtable, module, 0x3960, 0x3960);
	SetSharedMember<context, sq_newslotType>(manager, &manager->__sq_newslot, module, 0x70B0, 0x7080);

	// structs
	SetSharedMember<context, sq_pushnewstructinstanceType>(manager, &manager->__sq_pushnewstructinstance, module, 0x5400, 0x53e0);
	SetSharedMember<context, sq_sealstructslotType>(manager, &manager->__sq_sealstructslot, module, 0x5530, 0x5510);

	// push other objects to stack
	SetSharedMember<context, sq_pushstringType>(manager, &manager->__sq_pushstring, module, 0x3440, 0x3440);
	SetSharedMember<context, sq_pushintegerType>(manager, &manager->__sq_pushinteger, module, 0x36A0, 0x36A0);
	SetSharedMember<context, sq_pushfloatType>(manager, &manager->__sq_pushfloat, module, 0x3800, 0x3800);
	SetSharedMember<context, sq_pushboolType>(manager, &manager->__sq_pushbool, module, 0x3710, 0x3710);
	SetSharedMember<context, sq_pushassetType>(manager, &manager->__sq_pushasset, module, 0x3560, 0x3560);
	SetSharedMember<context, sq_pushvectorType>(manager, &manager->__sq_pushvector, module, 0x3780, 0x3780);
	SetSharedMember<context, sq_pushobjectType>(manager, &manager->__sq_pushobject, module, 0x83D0, 0x83A0);

	// get objects from stack
	SetSharedMember<context, sq_getstringType>(manager, &manager->__sq_getstring, module, 0x60C0, 0x60A0);
	SetSharedMember<context, sq_getintegerType>(manager, &manager->__sq_getinteger, module, 0x60E0, 0x60C0);
	SetSharedMember<context, sq_getfloatType>(manager, &manager->__sq_getfloat, module, 0x6100, 0x60E0);
	SetSharedMember<context, sq_getboolType>(manager, &manager->__sq_getbool, module, 0x6130, 0x6110);
	SetSharedMember<context, sq_getType>(manager, &manager->__sq_get, module, 0x7C30, 0x7C00);
	SetSharedMember<context, sq_getassetType>(manager, &manager->__sq_getasset, module, 0x6010, 0x5FF0);
	SetSharedMember<context, sq_getuserdataType>(manager, &manager->__sq_getuserdata, module, 0x63D0, 0x63B0);
	SetSharedMember<context, sq_getvectorType>(manager, &manager->__sq_getvector, module, 0x6140, 0x6120);
	SetSharedMember<context, sq_getthisentityType>(manager, &manager->__sq_getthisentity, module, 0x12F80, 0x203B0);
	SetSharedMember<context, sq_getobjectType>(manager, &manager->__sq_getobject, module, 0x6160, 0x6140);

	// userdata
	SetSharedMember<context, sq_createuserdataType>(manager, &manager->__sq_createuserdata, module, 0x38D0, 0x38D0);
	SetSharedMember<context, sq_setuserdatatypeidType>(manager, &manager->__sq_setuserdatatypeid, module, 0x6490, 0x6470);

	// entities
	SetSharedMember<context, sq_GetEntityConstantType>(manager, &manager->__sq_GetEntityConstant_CBaseEntity, module, 0x3E49B0, 0x418AF0);
	SetSharedMember<context, sq_getentityfrominstanceType>(manager, &manager->__sq_getentityfrominstance, module, 0x114F0, 0x1E920);

	// meta infos
	SetSharedMember<context, sq_getfunctionType>(manager, &manager->__sq_getfunction, module, 0x6CB0, 0x6C85);
	SetSharedMember<context, sq_stackinfosType>(manager, &manager->__sq_stackinfos, module, 0x35970, 0x35920);
}

ADD_SQFUNC(
	"string",
	NSGetCurrentModName,
	"",
	"Returns the mod name of the script running this function",
	ScriptContext::UI | ScriptContext::CLIENT | ScriptContext::SERVER)
{
	int depth = g_pSquirrel<context>->getinteger(sqvm, 1);
	if (auto mod = g_pSquirrel<context>->getcallingmod(sqvm, depth); mod == nullptr)
	{
		g_pSquirrel<context>->raiseerror(sqvm, "NSGetModName was called from a non-mod script. This shouldn't be possible");
		return SQRESULT_ERROR;
	}
	else
	{
		g_pSquirrel<context>->pushstring(sqvm, mod->Name.c_str());
	}
	return SQRESULT_NOTNULL;
}

ADD_SQFUNC(
	"string",
	NSGetCallingModName,
	"int depth = 0",
	"Returns the mod name of the script running this function",
	ScriptContext::UI | ScriptContext::CLIENT | ScriptContext::SERVER)
{
	int depth = g_pSquirrel<context>->getinteger(sqvm, 1);
	if (auto mod = g_pSquirrel<context>->getcallingmod(sqvm, depth); mod == nullptr)
	{
		g_pSquirrel<context>->pushstring(sqvm, "Unknown");
	}
	else
	{
		g_pSquirrel<context>->pushstring(sqvm, mod->Name.c_str());
	}
	return SQRESULT_NOTNULL;
}

ON_DLL_LOAD_RELIESON("client.dll", ClientSquirrel, ConCommand, (CModule module))
{
	AUTOHOOK_DISPATCH_MODULE(client.dll)

	InitSharedSquirrelFunctions<ScriptContext::CLIENT>(g_pSquirrel<ScriptContext::CLIENT>, module);

	g_pSquirrel<ScriptContext::UI>->messageBuffer = g_pSquirrel<ScriptContext::CLIENT>->messageBuffer;

	MAKEHOOK(
		module.Offset(0x108E0),
		&RegisterSquirrelFunctionHook<ScriptContext::CLIENT>,
		&g_pSquirrel<ScriptContext::CLIENT>->RegisterSquirrelFunc);
	g_pSquirrel<ScriptContext::UI>->RegisterSquirrelFunc = g_pSquirrel<ScriptContext::CLIENT>->RegisterSquirrelFunc;

	g_pSquirrel<ScriptContext::CLIENT>->logger = NS::log::SCRIPT_CL;
	g_pSquirrel<ScriptContext::UI>->logger = NS::log::SCRIPT_UI;

	// uiscript_reset concommand: don't loop forever if compilation fails
	module.Offset(0x3C6E4C).NOP(6);

	MAKEHOOK(module.Offset(0x8AD0), &sq_compiler_createHook<ScriptContext::CLIENT>, &sq_compiler_create<ScriptContext::CLIENT>);

	MAKEHOOK(module.Offset(0x12B00), &SQPrintHook<ScriptContext::CLIENT>, &SQPrint<ScriptContext::CLIENT>);
	MAKEHOOK(module.Offset(0x12BA0), &SQPrintHook<ScriptContext::UI>, &SQPrint<ScriptContext::UI>);

	MAKEHOOK(module.Offset(0x26130), &CreateNewVMHook<ScriptContext::CLIENT>, &CreateNewVM<ScriptContext::CLIENT>);
	MAKEHOOK(module.Offset(0x26E70), &DestroyVMHook<ScriptContext::CLIENT>, &DestroyVM<ScriptContext::CLIENT>);
	MAKEHOOK(module.Offset(0x79A50), &ScriptCompileErrorHook<ScriptContext::CLIENT>, &SQCompileError<ScriptContext::CLIENT>);

	MAKEHOOK(module.Offset(0x10190), &CallScriptInitCallbackHook<ScriptContext::CLIENT>, &CallScriptInitCallback<ScriptContext::CLIENT>);

	MAKEHOOK(module.Offset(0xE3B0), &CSquirrelVM_initHook<ScriptContext::CLIENT>, &CSquirrelVM_init<ScriptContext::CLIENT>);

	RegisterConCommand("script_client", ConCommand_script<ScriptContext::CLIENT>, "Executes script code on the client vm", FCVAR_CLIENTDLL);
	RegisterConCommand("script_ui", ConCommand_script<ScriptContext::UI>, "Executes script code on the ui vm", FCVAR_CLIENTDLL);

	StubUnsafeSQFuncs<ScriptContext::CLIENT>();
	StubUnsafeSQFuncs<ScriptContext::UI>();

	SquirrelFunctions s = {};
	g_pSquirrel<ScriptContext::CLIENT>->GenerateSquirrelFunctionsStruct(&s);
	g_pPluginManager->InformSQVMLoad(ScriptContext::CLIENT, &s);
}

ON_DLL_LOAD_RELIESON("server.dll", ServerSquirrel, ConCommand, (CModule module))
{
	AUTOHOOK_DISPATCH_MODULE(server.dll)

	InitSharedSquirrelFunctions<ScriptContext::SERVER>(g_pSquirrel<ScriptContext::SERVER>, module);

	g_pSquirrel<ScriptContext::SERVER>->logger = NS::log::SCRIPT_SV;

	MAKEHOOK(
		module.Offset(0x1DD10),
		&RegisterSquirrelFunctionHook<ScriptContext::SERVER>,
		&g_pSquirrel<ScriptContext::SERVER>->RegisterSquirrelFunc);

	MAKEHOOK(module.Offset(0x8AA0), &sq_compiler_createHook<ScriptContext::SERVER>, &sq_compiler_create<ScriptContext::SERVER>);

	MAKEHOOK(module.Offset(0x1FE90), &SQPrintHook<ScriptContext::SERVER>, &SQPrint<ScriptContext::SERVER>);
	MAKEHOOK(module.Offset(0x260E0), &CreateNewVMHook<ScriptContext::SERVER>, &CreateNewVM<ScriptContext::SERVER>);
	MAKEHOOK(module.Offset(0x26E20), &DestroyVMHook<ScriptContext::SERVER>, &DestroyVM<ScriptContext::SERVER>);
	MAKEHOOK(module.Offset(0x799E0), &ScriptCompileErrorHook<ScriptContext::SERVER>, &SQCompileError<ScriptContext::SERVER>);
	MAKEHOOK(module.Offset(0x1D5C0), &CallScriptInitCallbackHook<ScriptContext::SERVER>, &CallScriptInitCallback<ScriptContext::SERVER>);
	MAKEHOOK(module.Offset(0x17BE0), &CSquirrelVM_initHook<ScriptContext::SERVER>, &CSquirrelVM_init<ScriptContext::SERVER>);
	// FCVAR_CHEAT and FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS allows clients to execute this, but since it's unsafe we only allow it when cheats
	// are enabled for script_client and script_ui, we don't use cheats, so clients can execute them on themselves all they want
	RegisterConCommand(
		"script",
		ConCommand_script<ScriptContext::SERVER>,
		"Executes script code on the server vm",
		FCVAR_GAMEDLL | FCVAR_GAMEDLL_FOR_REMOTE_CLIENTS | FCVAR_CHEAT);

	StubUnsafeSQFuncs<ScriptContext::SERVER>();

	SquirrelFunctions s = {};
	g_pSquirrel<ScriptContext::SERVER>->GenerateSquirrelFunctionsStruct(&s);
	g_pPluginManager->InformSQVMLoad(ScriptContext::SERVER, &s);
}

void InitialiseSquirrelManagers()
{
	g_pSquirrel<ScriptContext::CLIENT> = new SquirrelManager<ScriptContext::CLIENT>;
	g_pSquirrel<ScriptContext::UI> = new SquirrelManager<ScriptContext::UI>;
	g_pSquirrel<ScriptContext::SERVER> = new SquirrelManager<ScriptContext::SERVER>;
}
