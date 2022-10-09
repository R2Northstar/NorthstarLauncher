#pragma once

#include "squirrelclasstypes.h"
#include "vector.h"

const char* GetContextName(ScriptContext context);
eSQReturnType SQReturnTypeFromString(const char* pReturnType);
const char* SQTypeNameFromID(const int iTypeId);

// core sqvm funcs
typedef int64_t (*RegisterSquirrelFuncType)(CSquirrelVM* sqvm, SQFuncRegistration* funcReg, char unknown);
typedef void (*sq_defconstType)(CSquirrelVM* sqvm, const SQChar* name, int value);

typedef SQRESULT (*sq_compilebufferType)(
	HSquirrelVM* sqvm, CompileBufferState* compileBuffer, const char* file, int a1, SQBool bShouldThrowError);
typedef SQRESULT (*sq_callType)(HSquirrelVM* sqvm, SQInteger iArgs, SQBool bShouldReturn, SQBool bThrowError);
typedef SQInteger (*sq_raiseerrorType)(HSquirrelVM* sqvm, const SQChar* pError);

// sq stack array funcs
typedef void (*sq_newarrayType)(HSquirrelVM* sqvm, SQInteger iStackpos);
typedef SQRESULT (*sq_arrayappendType)(HSquirrelVM* sqvm, SQInteger iStackpos);

// sq table funcs
typedef SQRESULT (*sq_newtableType)(HSquirrelVM* sqvm);
typedef SQRESULT (*sq_newslotType)(HSquirrelVM* sqvm, SQInteger idx, SQBool bStatic);

// sq stack push funcs
typedef void (*sq_pushroottableType)(HSquirrelVM* sqvm);
typedef void (*sq_pushstringType)(HSquirrelVM* sqvm, const SQChar* pStr, SQInteger iLength);
typedef void (*sq_pushintegerType)(HSquirrelVM* sqvm, SQInteger i);
typedef void (*sq_pushfloatType)(HSquirrelVM* sqvm, SQFloat f);
typedef void (*sq_pushboolType)(HSquirrelVM* sqvm, SQBool b);
typedef void (*sq_pushassetType)(HSquirrelVM* sqvm, const SQChar* str, SQInteger iLength);
typedef void (*sq_pushvectorType)(HSquirrelVM* sqvm, const SQFloat* pVec);
typedef void (*sq_pushSQObjectType)(HSquirrelVM* sqvm, SQObject* pVec);

// sq stack get funcs
typedef const SQChar* (*sq_getstringType)(HSquirrelVM* sqvm, SQInteger iStackpos);
typedef SQInteger (*sq_getintegerType)(HSquirrelVM* sqvm, SQInteger iStackpos);
typedef SQFloat (*sq_getfloatType)(HSquirrelVM*, SQInteger iStackpos);
typedef SQBool (*sq_getboolType)(HSquirrelVM*, SQInteger iStackpos);
typedef SQRESULT (*sq_getType)(HSquirrelVM* sqvm, SQInteger iStackpos);
typedef SQRESULT (*sq_getassetType)(HSquirrelVM* sqvm, SQInteger iStackpos, const char** pResult);
typedef SQRESULT (*sq_getuserdataType)(HSquirrelVM* sqvm, SQInteger iStackpos, void** pData, uint64_t* pTypeId);
typedef SQFloat* (*sq_getvectorType)(HSquirrelVM* sqvm, SQInteger iStackpos);

// sq stack userpointer funcs
typedef void* (*sq_createuserdataType)(HSquirrelVM* sqvm, SQInteger iSize);
typedef SQRESULT (*sq_setuserdatatypeidType)(HSquirrelVM* sqvm, SQInteger iStackpos, uint64_t iTypeId);

typedef int (*sq_getSquirrelFunctionType)(HSquirrelVM* sqvm, const char* name, SQObject* returnObj, const char* signature);

template <ScriptContext context> class SquirrelManager
{
  private:
	std::vector<SQFuncRegistration*> m_funcRegistrations;

  public:
	CSquirrelVM* m_pSQVM;
	std::map<std::string, SQFunction> m_funcOverrides = {};
	std::map<std::string, SQFunction> m_funcOriginals = {};

	bool m_bFatalCompilationErrors = false;

#pragma region SQVM funcs
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

#pragma endregion

#pragma region MessageBuffer
	SquirrelMessageBuffer* messageBuffer;
	sq_getSquirrelFunctionType __sq_getSquirrelFunction;

	template <typename... Args> SquirrelMessage createMessage(const char* funcname, Args... args)
	{
		int args_processed = 0;
		std::vector<std::function<void()>> function_vector;
		SqRecurseArgs<context>(function_vector, args...);
		SquirrelMessage message = {funcname, function_vector};
		messageBuffer->push(message);
		return message;
	}

#pragma endregion

  public:
	SquirrelManager() : m_pSQVM(nullptr) {}

	void VMCreated(CSquirrelVM* newSqvm);
	void VMDestroyed();
	void ExecuteCode(const char* code);
	void AddFuncRegistration(std::string returnType, std::string name, std::string argTypes, std::string helpText, SQFunction func);
	SQRESULT setupfunc(const SQChar* funcname);
	void AddFuncOverride(std::string name, SQFunction func);

#pragma region SQVM func wrappers
	inline void defconst(CSquirrelVM* sqvm, const SQChar* pName, int nValue)
	{
		__sq_defconst(sqvm, pName, nValue);
	}

	inline SQRESULT
	compilebuffer(CompileBufferState* bufferState, const SQChar* bufferName = "unnamedbuffer", const SQBool bShouldThrowError = false)
	{
		return __sq_compilebuffer(m_pSQVM->sqvm, bufferState, bufferName, -1, bShouldThrowError);
	}

	inline SQRESULT call(HSquirrelVM* sqvm, const SQInteger args)
	{
		return __sq_call(sqvm, args + 1, false, false);
	}

	inline SQInteger raiseerror(HSquirrelVM* sqvm, const const SQChar* sError)
	{
		return __sq_raiseerror(sqvm, sError);
	}

	inline void newarray(HSquirrelVM* sqvm, const SQInteger stackpos = 0)
	{
		__sq_newarray(sqvm, stackpos);
	}

	inline SQRESULT arrayappend(HSquirrelVM* sqvm, const SQInteger stackpos)
	{
		return __sq_arrayappend(sqvm, stackpos);
	}

	inline SQRESULT newtable(HSquirrelVM* sqvm)
	{
		return __sq_newtable(sqvm);
	}

	inline SQRESULT newslot(HSquirrelVM* sqvm, SQInteger idx, SQBool bStatic)
	{
		return __sq_newslot(sqvm, idx, bStatic);
	}

	inline void pushroottable(HSquirrelVM* sqvm)
	{
		__sq_pushroottable(sqvm);
	}

	inline void pushstring(HSquirrelVM* sqvm, const SQChar* sVal, int length = -1)
	{
		__sq_pushstring(sqvm, sVal, length);
	}

	inline void pushinteger(HSquirrelVM* sqvm, const SQInteger iVal)
	{
		__sq_pushinteger(sqvm, iVal);
	}

	inline void pushfloat(HSquirrelVM* sqvm, const SQFloat flVal)
	{
		__sq_pushfloat(sqvm, flVal);
	}

	inline void pushbool(HSquirrelVM* sqvm, const SQBool bVal)
	{
		__sq_pushbool(sqvm, bVal);
	}

	inline void pushasset(HSquirrelVM* sqvm, const SQChar* sVal, int length = -1)
	{
		__sq_pushasset(sqvm, sVal, length);
	}

	inline void pushvector(HSquirrelVM* sqvm, const Vector3 pVal)
	{
		__sq_pushvector(sqvm, *(float**)&pVal);
	}

	inline void pushSQObject(HSquirrelVM* sqvm, SQObject* obj)
	{
		__sq_pushSQObject(sqvm, obj);
	}

	inline const SQChar* getstring(HSquirrelVM* sqvm, const SQInteger stackpos)
	{
		return __sq_getstring(sqvm, stackpos);
	}

	inline SQInteger getinteger(HSquirrelVM* sqvm, const SQInteger stackpos)
	{
		return __sq_getinteger(sqvm, stackpos);
	}

	inline SQFloat getfloat(HSquirrelVM* sqvm, const SQInteger stackpos)
	{
		return __sq_getfloat(sqvm, stackpos);
	}

	inline SQBool getbool(HSquirrelVM* sqvm, const SQInteger stackpos)
	{
		return __sq_getbool(sqvm, stackpos);
	}

	inline SQRESULT get(HSquirrelVM* sqvm, const SQInteger stackpos)
	{
		return __sq_get(sqvm, stackpos);
	}

	inline Vector3 getvector(HSquirrelVM* sqvm, const SQInteger stackpos)
	{
		float* pRet = __sq_getvector(sqvm, stackpos);
		return *(Vector3*)&pRet;
	}

	inline int sq_getSquirrelFunction(HSquirrelVM* sqvm, const char* name, SQObject* returnObj, const char* signature) {
		return __sq_getSquirrelFunction(sqvm, name, returnObj, signature);
	}


	inline SQRESULT getasset(HSquirrelVM* sqvm, const SQInteger stackpos, const char** result)
	{
		return __sq_getasset(sqvm, stackpos, result);
	}

	template <typename T> inline SQRESULT getuserdata(HSquirrelVM* sqvm, const SQInteger stackpos, T* data, uint64_t* typeId)
	{
		return __sq_getuserdata(sqvm, stackpos, (void**)data, typeId); // this sometimes crashes idk
	}

	template <typename T> inline T* createuserdata(HSquirrelVM* sqvm, SQInteger size)
	{
		void* ret = __sq_createuserdata(sqvm, size);
		memset(ret, 0, size);
		return (T*)ret;
	}

	inline SQRESULT setuserdatatypeid(HSquirrelVM* sqvm, const SQInteger stackpos, uint64_t typeId)
	{
		return __sq_setuserdatatypeid(sqvm, stackpos, typeId);
	}
#pragma endregion
};

template <ScriptContext context> SquirrelManager<context>* g_pSquirrel;

#pragma region MessageBuffer templates


// clang-format off
// Base case for recursion
#ifndef MessageBufferFuncs
#define MessageBufferFuncs
inline void SQMessageBufferPushArg(ScriptContext context, FunctionVector& v)
{
	return;
}

template <ScriptContext context, typename T>
requires std::convertible_to<T, bool> && (!std::is_floating_point_v<T>) && (!std::convertible_to<T, std::string>) && (!std::convertible_to<T, int>)
inline void SQMessageBufferPushArg(FunctionVector& v, T& arg) {
	v.push_back([arg]() { g_pSquirrel<context>->pushbool(g_pSquirrel<context>->m_pSQVM->sqvm, static_cast<bool>(arg)); });
}

template <ScriptContext context, typename T>
requires std::convertible_to<T, int> && (!std::is_floating_point_v<T>)
inline void SQMessageBufferPushArg(FunctionVector& v, T& arg) {
	v.push_back([arg]() { g_pSquirrel<context>->pushinteger(g_pSquirrel<context>->m_pSQVM->sqvm, static_cast<int>(arg)); });
}

template <ScriptContext context, typename T>
requires std::convertible_to<T, float> && (std::is_floating_point_v<T>)
inline void SQMessageBufferPushArg(FunctionVector& v, T& arg) {
	v.push_back([arg]() { g_pSquirrel<context>->pushfloat(g_pSquirrel<context>->m_pSQVM->sqvm, static_cast<float>(arg)); });
}

template <ScriptContext context, typename T>
requires (std::convertible_to<T, std::string> || std::is_constructible_v<std::string, T>)
inline void SQMessageBufferPushArg(FunctionVector& v, T& arg) {
	auto converted = std::string(arg);
	v.push_back([converted]()
		{ g_pSquirrel<context>->pushstring(g_pSquirrel<context>->m_pSQVM->sqvm, converted.c_str(), converted.length()); });
}

template <ScriptContext context>
inline void SQMessageBufferPushArg(FunctionVector& v, SquirrelAsset& arg) {
	v.push_back([arg]()
		{ g_pSquirrel<context>->pushasset(g_pSquirrel<context>->m_pSQVM->sqvm, arg.path.c_str(), arg.path.length()); });
}

template <ScriptContext context, typename T>
requires is_iterable<T>
inline void SQMessageBufferPushArg(FunctionVector& v, T& arg) {
	FunctionVector localv = {};
	localv.push_back([]{g_pSquirrel<context>->newarray(g_pSquirrel<context>->m_pSQVM->sqvm, 0);});
	
	for (const auto& item : arg) {
		SQMessageBufferPushArg<context>(localv, item);
		localv.push_back([]{g_pSquirrel<context>->arrayappend(g_pSquirrel<context>->m_pSQVM->sqvm, -2);});
	}

	v.push_back([localv] {
		for (auto& func : localv) {
			func();
		}
	});
}

template <ScriptContext context, typename T>
requires is_map<T>
inline  void SQMessageBufferPushArg(FunctionVector& v, T& map) {
	FunctionVector localv = {};
	localv.push_back([]{g_pSquirrel<context>->newtable(g_pSquirrel<context>->m_pSQVM->sqvm);});
	
	for (const auto& item : map) {
		SQMessageBufferPushArg<context>(localv, item.first);
		SQMessageBufferPushArg<context>(localv, item.second);
		localv.push_back([]{g_pSquirrel<context>->newslot(g_pSquirrel<context>->m_pSQVM->sqvm, -3, false);});
	}

	v.push_back([localv] {
		for (auto& func : localv) {
			func();
		}
	});
}

template <ScriptContext context, typename T, typename... Args>
inline void SqRecurseArgs(FunctionVector& v, T& arg, Args... args) {
	SQMessageBufferPushArg<context>(v, arg);
	SqRecurseArgs(v, args...);
}

template <ScriptContext context, typename T>
inline void SqRecurseArgs(FunctionVector& v, T& arg) {
	SQMessageBufferPushArg<context>(v, arg);
}

// clang-format on
#endif 

#pragma endregion
