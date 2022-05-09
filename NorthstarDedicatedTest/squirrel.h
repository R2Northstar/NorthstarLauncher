#pragma once

// stolen from ttf2sdk: sqvm types
typedef float SQFloat;
typedef long SQInteger;
typedef unsigned long SQUnsignedInteger;
typedef char SQChar;

typedef SQUnsignedInteger SQBool;

enum SQRESULT : SQInteger
{
	SQRESULT_ERROR = -1,
	SQRESULT_NULL = 0,
	SQRESULT_NOTNULL = 1,
};

typedef SQRESULT (*SQFunction)(void* sqvm);

struct CompileBufferState
{
	const SQChar* buffer;
	const SQChar* bufferPlusLength;
	const SQChar* bufferAgain;

	CompileBufferState(const std::string& code)
	{
		buffer = code.c_str();
		bufferPlusLength = code.c_str() + code.size();
		bufferAgain = code.c_str();
	}
};

struct SQFuncRegistration
{
	const char* squirrelFuncName;
	const char* cppFuncName;
	const char* helpText;
	const char* returnValueType;
	const char* argTypes;
	int16_t somethingThatsZero;
	int16_t padding1;
	int32_t unknown1;
	int64_t unknown2;
	int32_t unknown3;
	int32_t padding2;
	int64_t unknown4;
	int64_t unknown5;
	int64_t unknown6;
	int32_t unknown7;
	int32_t padding3;
	void* funcPtr;

	SQFuncRegistration()
	{
		memset(this, 0, sizeof(SQFuncRegistration));
		this->padding2 = 32;
	}
};

// core sqvm funcs
typedef int64_t (*RegisterSquirrelFuncType)(void* sqvm, SQFuncRegistration* funcReg, char unknown);
extern RegisterSquirrelFuncType ClientRegisterSquirrelFunc;
extern RegisterSquirrelFuncType ServerRegisterSquirrelFunc;

typedef SQRESULT (*sq_compilebufferType)(void* sqvm, CompileBufferState* compileBuffer, const char* file, int a1, ScriptContext a2);
typedef SQRESULT (*sq_callType)(void* sqvm, SQInteger s1, SQBool a2, SQBool a3);

// sq stack array funcs
typedef void (*sq_newarrayType)(void* sqvm, SQInteger stackpos);
typedef SQRESULT (*sq_arrayappendType)(void* sqvm, SQInteger stackpos);

// sq stack push funcs
typedef void (*sq_pushroottableType)(void* sqvm);
typedef void (*sq_pushstringType)(void* sqvm, const SQChar* str, SQInteger length);
typedef void (*sq_pushintegerType)(void* sqvm, SQInteger i);
typedef void (*sq_pushfloatType)(void* sqvm, SQFloat f);
typedef void (*sq_pushboolType)(void* sqvm, SQBool b);
typedef SQInteger (*sq_raiseerrorType)(void* sqvm, const SQChar* error);

// sq stack get funcs
typedef const SQChar* (*sq_getstringType)(void* sqvm, SQInteger stackpos);
typedef SQInteger (*sq_getintegerType)(void* sqvm, SQInteger stackpos);
typedef SQFloat (*sq_getfloatType)(void*, SQInteger stackpos);
typedef SQBool (*sq_getboolType)(void*, SQInteger stackpos);
typedef SQRESULT (*sq_getType)(void* sqvm, SQInteger stackpos);

template <ScriptContext context> class SquirrelManager
{
  private:
	std::vector<SQFuncRegistration*> m_funcRegistrations;

  public:
	void* sqvm;
	void* sqvm2;
	#pragma region SQVM funcs
	sq_compilebufferType sq_compilebuffer;
	sq_callType sq_call;
	sq_newarrayType sq_newarray;
	sq_arrayappendType sq_arrayappend;
	sq_pushroottableType sq_pushroottable;
	sq_pushstringType sq_pushstring;
	sq_pushintegerType sq_pushinteger;
	sq_pushfloatType sq_pushfloat;
	sq_pushboolType sq_pushbool;
	sq_raiseerrorType sq_raiseerror;

	sq_getstringType sq_getstring;
	sq_getintegerType sq_getinteger;
	sq_getfloatType sq_getfloat;
	sq_getboolType sq_getbool;
	sq_getType sq_get;
	#pragma endregion

  public:
	SquirrelManager() : sqvm(nullptr) {}

	void VMCreated(void* newSqvm)
	{
		sqvm = newSqvm;
		sqvm2 = *((void**)((char*)sqvm + 8)); // honestly not 100% sure on what this is, but alot of functions take it

		for (SQFuncRegistration* funcReg : m_funcRegistrations)
		{
			spdlog::info("Registering {} function {}", GetContextName(context), funcReg->squirrelFuncName);

			if (context == ScriptContext::CLIENT || context == ScriptContext::UI)
				ClientRegisterSquirrelFunc(sqvm, funcReg, 1);
			else
				ServerRegisterSquirrelFunc(sqvm, funcReg, 1);
		}
	}

	void VMDestroyed()
	{
		sqvm = nullptr;
	}

	void ExecuteCode(const char* code)
	{
		if (!sqvm)
		{
			spdlog::error("Cannot execute code, {} squirrel vm is not initialised", GetContextName(context));
			return;
		}

		spdlog::info("Executing {} script code {} ", GetContextName(context), code);

		std::string strCode(code);
		CompileBufferState bufferState = CompileBufferState(strCode);

		SQRESULT compileResult = sq_compilebuffer(sqvm2, &bufferState, "console", -1, context);
		spdlog::info("sq_compilebuffer returned {}", compileResult);
		if (compileResult >= 0)
		{
			sq_pushroottable(sqvm2);
			SQRESULT callResult = sq_call(sqvm2, 1, false, false);
			spdlog::info("sq_call returned {}", callResult);
		}
	}

	int setupfunc(const char* funcname)
	{
		sq_pushroottable(sqvm2);
		sq_pushstring(sqvm2, funcname, -1);

		int result = sq_get(sqvm2, -2);
		if (result != SQRESULT_ERROR)
			sq_pushroottable(sqvm2);

		return result;
	}

	void pusharg(int arg)
	{
		sq_pushinteger(sqvm2, arg);
	}

	void pusharg(const char* arg)
	{
		sq_pushstring(sqvm2, arg, -1);
	}

	void pusharg(float arg)
	{
		sq_pushfloat(sqvm2, arg);
	}

	void pusharg(bool arg)
	{
		sq_pushbool(sqvm2, arg);
	}

	int call(int args)
	{
		return sq_call(sqvm2, args + 1, false, false);
	}

	void AddFuncRegistration(std::string returnType, std::string name, std::string argTypes, std::string helpText, SQFunction func)
	{
		SQFuncRegistration* reg = new SQFuncRegistration;

		reg->squirrelFuncName = new char[name.size() + 1];
		strcpy((char*)reg->squirrelFuncName, name.c_str());
		reg->cppFuncName = reg->squirrelFuncName;

		reg->helpText = new char[helpText.size() + 1];
		strcpy((char*)reg->helpText, helpText.c_str());

		reg->returnValueType = new char[returnType.size() + 1];
		strcpy((char*)reg->returnValueType, returnType.c_str());

		reg->argTypes = new char[argTypes.size() + 1];
		strcpy((char*)reg->argTypes, argTypes.c_str());

		reg->funcPtr = func;

		m_funcRegistrations.push_back(reg);
	}
};

extern SquirrelManager<ScriptContext::CLIENT>* g_ClientSquirrelManager;
extern SquirrelManager<ScriptContext::SERVER>* g_ServerSquirrelManager;
extern SquirrelManager<ScriptContext::UI>* g_UISquirrelManager;
