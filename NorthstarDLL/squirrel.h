#pragma once

// stolen from ttf2sdk: sqvm types
typedef float SQFloat;
typedef long SQInteger;
typedef unsigned long SQUnsignedInteger;
typedef char SQChar;
typedef SQUnsignedInteger SQBool;

int GetReturnTypeEnumFromString(const char* name);

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


enum SQReturnTypeEnum {
	SqReturnFloat = 0x1,
	SqReturnVector = 0x3,
	SqReturnInteger = 0x5,
	SqReturnBoolean = 0x6,
	SqReturnEntity = 0xD,
	SqReturnString = 0x21,
	SqReturnDefault = 0x20,
	SqReturnArrays = 0x25,
	SqReturnAsset = 0x28,
	SqReturnTable = 0x26,
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
	int32_t returnValueTypeEnum;
	int64_t unknown4;
	int64_t unknown5;
	int64_t unknown6;
	int32_t unknown7;
	int32_t padding3;
	void* funcPtr;

	SQFuncRegistration()
	{
		memset(this, 0, sizeof(SQFuncRegistration));
	}
};

enum class ScriptContext : int
{
	SERVER,
	CLIENT,
	UI,
	NONE
};

const char* GetContextName(ScriptContext context);

// core sqvm funcs
typedef int64_t (*RegisterSquirrelFuncType)(void* sqvm, SQFuncRegistration* funcReg, char unknown);

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
	RegisterSquirrelFuncType RegisterSquirrelFunc;

	sq_compilebufferType __sq_compilebuffer;
	sq_callType __sq_call;
	sq_raiseerrorType __sq_raiseerror;

	sq_newarrayType __sq_newarray;
	sq_arrayappendType __sq_arrayappend;

	sq_pushroottableType __sq_pushroottable;
	sq_pushstringType __sq_pushstring;
	sq_pushintegerType __sq_pushinteger;
	sq_pushfloatType __sq_pushfloat;
	sq_pushboolType __sq_pushbool;

	sq_getstringType __sq_getstring;
	sq_getintegerType __sq_getinteger;
	sq_getfloatType __sq_getfloat;
	sq_getboolType __sq_getbool;
	sq_getType __sq_get;
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
			RegisterSquirrelFunc(sqvm, funcReg, 1);
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

		SQRESULT compileResult = compilebuffer(&bufferState, "console");
		spdlog::info("sq_compilebuffer returned {}", compileResult);
		if (compileResult != SQRESULT_ERROR)
		{
			pushroottable(sqvm2);
			SQRESULT callResult = call(sqvm2, 0);
			spdlog::info("sq_call returned {}", callResult);
		}
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
		reg->returnValueTypeEnum = GetReturnTypeEnumFromString(returnType.c_str());

		m_funcRegistrations.push_back(reg);
	}

	SQRESULT setupfunc(const SQChar* funcname)
	{
		pushroottable(sqvm2);
		pushstring(sqvm2, funcname, -1);

		SQRESULT result = get(sqvm2, -2);
		if (result != SQRESULT_ERROR)
			pushroottable(sqvm2);

		return result;
	}

	#pragma region SQVM func wrappers
	SQRESULT compilebuffer(CompileBufferState* bufferState, const SQChar* bufferName = "unnamedbuffer")
	{
		return __sq_compilebuffer(sqvm2, bufferState, bufferName, -1, context);
	}

	SQRESULT call(void* sqvm, const SQInteger args)
	{
		return __sq_call(sqvm, args + 1, false, false);
	}

	SQInteger raiseerror(void* sqvm, const const SQChar* sError)
	{
		return __sq_raiseerror(sqvm, sError);
	}

	void newarray(void* sqvm, const SQInteger stackpos = 0) 
	{
		__sq_newarray(sqvm, stackpos);
	}

	SQRESULT arrayappend(void* sqvm, const SQInteger stackpos) 
	{
		return __sq_arrayappend(sqvm, stackpos);
	}

	void pushroottable(void* sqvm)
	{
		__sq_pushroottable(sqvm);
	}

	void pushstring(void* sqvm, const SQChar* sVal, int length = -1)
	{
		__sq_pushstring(sqvm, sVal, length);
	}

	void pushinteger(void* sqvm, const SQInteger iVal)
	{
		__sq_pushinteger(sqvm, iVal);
	}

	void pushfloat(void* sqvm, const SQFloat flVal)
	{
		__sq_pushfloat(sqvm, flVal);
	}

	void pushbool(void* sqvm, const SQBool bVal)
	{
		__sq_pushbool(sqvm, bVal);
	}

	const SQChar* getstring(void* sqvm, const SQInteger stackpos)
	{
		return __sq_getstring(sqvm, stackpos);
	}

	SQInteger getinteger(void* sqvm, const SQInteger stackpos)
	{
		return __sq_getinteger(sqvm, stackpos);
	}

	SQFloat getfloat(void* sqvm, const SQInteger stackpos)
	{
		return __sq_getfloat(sqvm, stackpos);
	}

	SQBool getbool(void* sqvm, const SQInteger stackpos)
	{
		return __sq_getbool(sqvm, stackpos);
	}

	SQRESULT get(void* sqvm, const SQInteger stackpos)
	{
		return __sq_get(sqvm, stackpos);
	}
	#pragma endregion
};

extern SquirrelManager<ScriptContext::CLIENT>* g_pClientSquirrel;
extern SquirrelManager<ScriptContext::SERVER>* g_pServerSquirrel;
extern SquirrelManager<ScriptContext::UI>* g_pUISquirrel;
template <ScriptContext context> SquirrelManager<context>* GetSquirrelManager();
