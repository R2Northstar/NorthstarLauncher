#pragma once

#include "squirreldatatypes.h"
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

const std::map<SQRESULT, const char*> PrintSQRESULT = {
	{SQRESULT_ERROR, "SQRESULT_ERROR"},
	{SQRESULT_NULL, "SQRESULT_NULL"},
	{SQRESULT_NOTNULL, "SQRESULT_NOTNULL"}
};

enum SQReturnTypeEnum
{
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

typedef SQRESULT (*SQFunction)(HSquirrelVM* sqvm);

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
	const char* returnTypeString;
	const char* argTypes;
	__int32 unknown1;
	__int32 devLevel;
	const char* shortNameMaybe;
	__int32 unknown2;
	SQReturnTypeEnum returnTypeEnum;
	__int32* externalBufferPointer;
	__int64 externalBufferSize;
	__int64 unknown3;
	__int64 unknown4;
	SQFunction funcPtr;

	SQFuncRegistration()
	{
		memset(this, 0, sizeof(SQFuncRegistration));
		this->returnTypeEnum = SqReturnDefault;
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
SQReturnTypeEnum GetReturnTypeEnumFromString(const char* returnTypeString);
const char* sq_getTypeName(int type);

// core sqvm funcs
typedef int64_t (*RegisterSquirrelFuncType)(CSquirrelVM* sqvm, SQFuncRegistration* funcReg, char unknown);

typedef SQRESULT (*sq_compilebufferType)(HSquirrelVM* sqvm, CompileBufferState* compileBuffer, const char* file, int a1, SQBool bShouldThrowError);
typedef SQRESULT (*sq_callType)(HSquirrelVM* sqvm, SQInteger iArgs, SQBool bShouldReturn, SQBool bThrowError);

// sq stack array funcs
typedef void (*sq_newarrayType)(HSquirrelVM* sqvm, SQInteger iStackpos);
typedef SQRESULT (*sq_arrayappendType)(HSquirrelVM* sqvm, SQInteger iStackpos);

// sq stack push funcs
typedef void (*sq_pushroottableType)(HSquirrelVM* sqvm);
typedef void (*sq_pushstringType)(HSquirrelVM* sqvm, const SQChar* pStr, SQInteger iLength);
typedef void (*sq_pushintegerType)(HSquirrelVM* sqvm, SQInteger i);
typedef void (*sq_pushfloatType)(HSquirrelVM* sqvm, SQFloat f);
typedef void (*sq_pushboolType)(HSquirrelVM* sqvm, SQBool b);
typedef void (*sq_pushassetType)(HSquirrelVM* sqvm, const SQChar* str, SQInteger iLength);
typedef void (*sq_pushvectorType)(HSquirrelVM* sqvm, float[3] pVec);
typedef SQInteger (*sq_raiseerrorType)(HSquirrelVM* sqvm, const SQChar* pError);

// sq stack get funcs
typedef const SQChar* (*sq_getstringType)(HSquirrelVM* sqvm, SQInteger iStackpos);
typedef SQInteger (*sq_getintegerType)(HSquirrelVM* sqvm, SQInteger iStackpos);
typedef SQFloat (*sq_getfloatType)(HSquirrelVM*, SQInteger iStackpos);
typedef SQBool (*sq_getboolType)(HSquirrelVM*, SQInteger iStackpos);
typedef SQRESULT (*sq_getType)(HSquirrelVM* sqvm, SQInteger iStackpos);
typedef SQRESULT (*sq_getassetType)(HSquirrelVM* sqvm, SQInteger iStackpos, const char** pResult);
typedef SQRESULT (*sq_getuserdataType)(HSquirrelVM* sqvm, SQInteger iStackpos, void** pData, long long* pTypeId);
typedef SQFloat* (*sq_getvectorType)(HSquirrelVM* sqvm, SQInteger iStackpos);

// sq stack userpointer funcs
typedef void* (*sq_createuserdataType)(HSquirrelVM* sqvm, SQInteger iSize);
typedef SQRESULT (*sq_setuserdatatypeidType)(HSquirrelVM* sqvm, SQInteger iStackpos, long long iTypeId);

template <ScriptContext context> class SquirrelManager
{
  private:
	std::vector<SQFuncRegistration*> m_funcRegistrations;

  public:
	CSquirrelVM* SquirrelVM;
	HSquirrelVM* sqvm;
	std::map<std::string, SQFunction> m_funcOverrides;
	std::map<std::string, SQFunction> m_funcOriginals;

	bool m_bCompilationErrorsFatal = false;

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
	sq_pushassetType __sq_pushasset;
	sq_pushvectorType __sq_pushvector;

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
#pragma endregion

  public:
	SquirrelManager() : SquirrelVM(nullptr) {}

	void VMCreated(CSquirrelVM* newSqvm)
	{
		SquirrelVM = newSqvm; // SquirrelManager in vanilla
		sqvm = newSqvm->sqvm; // actual SquirrelVM

		for (SQFuncRegistration* funcReg : m_funcRegistrations)
		{
			spdlog::info("Registering {} function {}", GetContextName(context), funcReg->squirrelFuncName);
			RegisterSquirrelFunc(SquirrelVM, funcReg, 1);
		}
	}

	void VMDestroyed()
	{
		SquirrelVM = nullptr;
	}

	void ExecuteCode(const char* code)
	{
		if (!SquirrelVM)
		{
			spdlog::error("Cannot execute code, {} squirrel vm is not initialised", GetContextName(context));
			return;
		}

		spdlog::info("Executing {} script code {} ", GetContextName(context), code);

		std::string strCode(code);
		CompileBufferState bufferState = CompileBufferState(strCode);

		SQRESULT compileResult = compilebuffer(&bufferState, "console");
		spdlog::info("sq_compilebuffer returned {}", PrintSQRESULT.at(compileResult));

		if (compileResult != SQRESULT_ERROR)
		{
			pushroottable(sqvm);
			SQRESULT callResult = call(sqvm, 0);
			spdlog::info("sq_call returned {}", PrintSQRESULT.at(callResult));
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

		reg->returnTypeString = new char[returnType.size() + 1];
		strcpy((char*)reg->returnTypeString, returnType.c_str());

		reg->argTypes = new char[argTypes.size() + 1];
		strcpy((char*)reg->argTypes, argTypes.c_str());

		reg->funcPtr = func;

		reg->returnTypeEnum = GetReturnTypeEnumFromString(returnType.c_str());

		m_funcRegistrations.push_back(reg);
	}

	void AddFuncOverride(std::string name, SQFunction func)
	{
		m_funcOverrides[name] = func;
	}

	SQRESULT setupfunc(const SQChar* funcname)
	{
		pushroottable(sqvm);
		pushstring(sqvm, funcname, -1);

		SQRESULT result = get(sqvm, -2);
		if (result != SQRESULT_ERROR)
			pushroottable(sqvm);

		return result;
	}

	#pragma region SQVM func wrappers
	SQRESULT compilebuffer(CompileBufferState* bufferState, const SQChar* bufferName = "unnamedbuffer", const SQBool bShouldThrowError = false)
	{
		return __sq_compilebuffer(sqvm, bufferState, bufferName, -1, bShouldThrowError);
	}

	SQRESULT call(HSquirrelVM* sqvm, const SQInteger args)
	{
		return __sq_call(sqvm, args + 1, false, false);
	}

	SQInteger raiseerror(HSquirrelVM* sqvm, const const SQChar* sError)
	{
		return __sq_raiseerror(sqvm, sError);
	}

	void newarray(HSquirrelVM* sqvm, const SQInteger stackpos = 0)
	{
		__sq_newarray(sqvm, stackpos);
	}

	SQRESULT arrayappend(HSquirrelVM* sqvm, const SQInteger stackpos)
	{
		return __sq_arrayappend(sqvm, stackpos);
	}

	void pushroottable(HSquirrelVM* sqvm)
	{
		__sq_pushroottable(sqvm);
	}

	void pushstring(HSquirrelVM* sqvm, const SQChar* sVal, int length = -1)
	{
		__sq_pushstring(sqvm, sVal, length);
	}

	void pushinteger(HSquirrelVM* sqvm, const SQInteger iVal)
	{
		__sq_pushinteger(sqvm, iVal);
	}

	void pushfloat(HSquirrelVM* sqvm, const SQFloat flVal)
	{
		__sq_pushfloat(sqvm, flVal);
	}

	void pushbool(HSquirrelVM* sqvm, const SQBool bVal)
	{
		__sq_pushbool(sqvm, bVal);
	}

	void pushasset(HSquirrelVM* sqvm, const SQChar* aVal, int length = -1)
	{
		__sq_pushasset(sqvm, aVal, length);
	}

	void pushvector(HSquirrelVM* sqvm, SQFloat* vVal)
	{
		__sq_pushvector(sqvm, vVal);
	}

	const SQChar* getstring(HSquirrelVM* sqvm, const SQInteger stackpos)
	{
		return __sq_getstring(sqvm, stackpos);
	}

	SQInteger getinteger(HSquirrelVM* sqvm, const SQInteger stackpos)
	{
		return __sq_getinteger(sqvm, stackpos);
	}

	SQFloat getfloat(HSquirrelVM* sqvm, const SQInteger stackpos)
	{
		return __sq_getfloat(sqvm, stackpos);
	}

	SQBool getbool(HSquirrelVM* sqvm, const SQInteger stackpos)
	{
		return __sq_getbool(sqvm, stackpos);
	}

	SQRESULT get(HSquirrelVM* sqvm, const SQInteger stackpos)
	{
		return __sq_get(sqvm, stackpos);
	}

	SQFloat* getvector(HSquirrelVM* sqvm, const SQInteger stackpos)
	{
		return __sq_getvector(sqvm, stackpos);
	}

	SQRESULT getasset(HSquirrelVM* sqvm, const SQInteger stackpos, const char** result)
	{
		return __sq_getasset(sqvm, stackpos, result);
	}

	SQRESULT getuserdata(HSquirrelVM* sqvm, const SQInteger stackpos, void** data, long long* typeId)
	{
		return __sq_getuserdata(sqvm, stackpos, data, typeId); // this sometimes crashes idk
	}

	void* createuserdata(HSquirrelVM* sqvm, SQInteger size)
	{
		void* ret = __sq_createuserdata(sqvm, size);
		memset(ret, 0, size);
		return ret;
	}

	SQRESULT setuserdatatypeid(HSquirrelVM* sqvm, const SQInteger stackpos, long long typeId)
	{
		return __sq_setuserdatatypeid(sqvm, stackpos, typeId);
	}
#pragma endregion
};

template <ScriptContext context> SquirrelManager<context>* g_pSquirrel;
