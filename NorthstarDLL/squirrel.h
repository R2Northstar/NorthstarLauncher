#pragma once

#include "squirrelclasstypes.h"
#include "squirrelautobind.h"
#include "vector.h"

// stolen from ttf2sdk: sqvm types
typedef float SQFloat;
typedef long SQInteger;
typedef unsigned long SQUnsignedInteger;
typedef char SQChar;
typedef SQUnsignedInteger SQBool;

static constexpr int operator&(ScriptContext first, ScriptContext second)
{
	return first == second;
}

static constexpr int operator&(int first, ScriptContext second)
{
	return first & (1 << static_cast<int>(second));
}

static constexpr int operator|(ScriptContext first, ScriptContext second)
{
	return (1 << static_cast<int>(first)) + (1 << static_cast<int>(second));
}

static constexpr int operator|(int first, ScriptContext second)
{
	return first + (1 << static_cast<int>(second));
}

#include "squirrelclasstypes.h"
#include "vector.h"

const char* GetContextName(ScriptContext context);
const char* GetContextName_Short(ScriptContext context);
eSQReturnType SQReturnTypeFromString(const char* pReturnType);
const char* SQTypeNameFromID(const int iTypeId);

void schedule_call_external(ScriptContext context, const char* func_name, SquirrelMessage_External_Pop function);

// This base class means that only the templated functions have to be rebuilt for each template instance
// Cuts down on compile time by ~5 seconds
class SquirrelManagerBase
{
  public:
	std::vector<SQFuncRegistration*> m_funcRegistrations;

	CSquirrelVM* m_pSQVM;
	std::map<std::string, SQFunction> m_funcOverrides = {};
	std::map<std::string, SQFunction> m_funcOriginals = {};

	bool m_bFatalCompilationErrors = false;

	std::shared_ptr<spdlog::logger> logger;

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
	sq_getthisentityType __sq_getthisentity;
	sq_getobjectType __sq_getobject;

	sq_createuserdataType __sq_createuserdata;
	sq_setuserdatatypeidType __sq_setuserdatatypeid;
	sq_getSquirrelFunctionType __sq_getSquirrelFunction;

	sq_getentityfrominstanceType __sq_getentityfrominstance;
	sq_GetEntityConstantType __sq_GetEntityConstant_CBaseEntity;

#pragma endregion

#pragma region SQVM func wrappers
	inline virtual void defconst(CSquirrelVM* sqvm, const SQChar* pName, int nValue) final
	{
		__sq_defconst(sqvm, pName, nValue);
	}

	inline virtual SQRESULT
	compilebuffer(CompileBufferState* bufferState, const SQChar* bufferName = "unnamedbuffer", const SQBool bShouldThrowError = false) final
	{
		return __sq_compilebuffer(m_pSQVM->sqvm, bufferState, bufferName, -1, bShouldThrowError);
	}

	inline virtual SQRESULT _call(HSquirrelVM* sqvm, const SQInteger args) final
	{
		return __sq_call(sqvm, args + 1, false, false);
	}

	inline virtual SQInteger raiseerror(HSquirrelVM* sqvm, const const SQChar* sError) final
	{
		return __sq_raiseerror(sqvm, sError);
	}

	inline virtual void newarray(HSquirrelVM* sqvm, const SQInteger stackpos = 0) final
	{
		__sq_newarray(sqvm, stackpos);
	}

	inline virtual SQRESULT arrayappend(HSquirrelVM* sqvm, const SQInteger stackpos) final
	{
		return __sq_arrayappend(sqvm, stackpos);
	}

	inline virtual SQRESULT newtable(HSquirrelVM* sqvm) final
	{
		return __sq_newtable(sqvm);
	}

	inline virtual SQRESULT newslot(HSquirrelVM* sqvm, SQInteger idx, SQBool bStatic) final
	{
		return __sq_newslot(sqvm, idx, bStatic);
	}

	inline virtual void pushroottable(HSquirrelVM* sqvm) final
	{
		__sq_pushroottable(sqvm);
	}

	inline virtual void pushstring(HSquirrelVM* sqvm, const SQChar* sVal, int length = -1) final
	{
		__sq_pushstring(sqvm, sVal, length);
	}

	inline virtual void pushinteger(HSquirrelVM* sqvm, const SQInteger iVal) final
	{
		__sq_pushinteger(sqvm, iVal);
	}

	inline virtual void pushfloat(HSquirrelVM* sqvm, const SQFloat flVal) final
	{
		__sq_pushfloat(sqvm, flVal);
	}

	inline virtual void pushbool(HSquirrelVM* sqvm, const SQBool bVal) final
	{
		__sq_pushbool(sqvm, bVal);
	}

	inline virtual void pushasset(HSquirrelVM* sqvm, const SQChar* sVal, int length = -1) final
	{
		__sq_pushasset(sqvm, sVal, length);
	}

	inline virtual void pushvector(HSquirrelVM* sqvm, const Vector3 pVal) final
	{
		__sq_pushvector(sqvm, *(float**)&pVal);
	}

	inline virtual void pushSQObject(HSquirrelVM* sqvm, SQObject* obj) final
	{
		__sq_pushSQObject(sqvm, obj);
	}

	inline virtual const SQChar* getstring(HSquirrelVM* sqvm, const SQInteger stackpos) final
	{
		return __sq_getstring(sqvm, stackpos);
	}

	inline virtual SQInteger getinteger(HSquirrelVM* sqvm, const SQInteger stackpos) final
	{
		return __sq_getinteger(sqvm, stackpos);
	}

	inline virtual SQFloat getfloat(HSquirrelVM* sqvm, const SQInteger stackpos) final
	{
		return __sq_getfloat(sqvm, stackpos);
	}

	inline virtual SQBool getbool(HSquirrelVM* sqvm, const SQInteger stackpos) final
	{
		return __sq_getbool(sqvm, stackpos);
	}

	inline virtual SQRESULT get(HSquirrelVM* sqvm, const SQInteger stackpos) final
	{
		return __sq_get(sqvm, stackpos);
	}

	inline virtual Vector3 getvector(HSquirrelVM* sqvm, const SQInteger stackpos) final
	{
		float* pRet = __sq_getvector(sqvm, stackpos);
		return *(Vector3*)&pRet;
	}

	inline virtual int sq_getSquirrelFunction(HSquirrelVM* sqvm, const char* name, SQObject* returnObj, const char* signature) final
	{
		return __sq_getSquirrelFunction(sqvm, name, returnObj, signature);
	}

	inline virtual SQRESULT getasset(HSquirrelVM* sqvm, const SQInteger stackpos, const char** result) final
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

	inline virtual SQRESULT setuserdatatypeid(HSquirrelVM* sqvm, const SQInteger stackpos, uint64_t typeId) final
	{
		return __sq_setuserdatatypeid(sqvm, stackpos, typeId);
	}

	template <typename T> inline SQBool getthisentity(HSquirrelVM* sqvm, T* ppEntity)
	{
		return __sq_getentity(sqvm, (void**)ppEntity);
	}

	template <typename T> inline T* getentity(HSquirrelVM* sqvm, SQInteger iStackPos)
	{
		SQObject obj;
		__sq_getobject(sqvm, iStackPos, &obj);

		// there are entity constants for other types, but seemingly CBaseEntity's is the only one needed
		return (T*)__sq_getentityfrominstance(m_pSQVM, &obj, __sq_GetEntityConstant_CBaseEntity());
	}
#pragma endregion
};

template <ScriptContext context> class SquirrelManager : public virtual SquirrelManagerBase
{
  public:
#pragma region MessageBuffer
	SquirrelMessageBuffer* messageBuffer;

	template <typename... Args> SquirrelMessage schedule_call(std::string funcname, Args... args)
	{
		// This function schedules a call to be executed on the next frame
		// This is useful for things like threads and plugins, which do not run on the main thread
		FunctionVector function_vector;
		SqRecurseArgs<context>(function_vector, args...);
		SquirrelMessage message = {funcname, function_vector};
		messageBuffer->push(message);
		return message;
	}

	SquirrelMessage schedule_call(std::string funcname)
	{
		// This function schedules a call to be executed on the next frame
		// This is useful for things like threads and plugins, which do not run on the main thread
		FunctionVector function_vector = {};
		SquirrelMessage message = {funcname, function_vector};
		messageBuffer->push(message);
		return message;
	}

	SQRESULT call(const char* funcname)
	{
		// Warning!
		// This function assumes the squirrel VM is stopped/blocked at the moment of call
		// Calling this function while the VM is running is likely to result in a crash due to stack destruction
		// If you want to call into squirrel asynchronously, use `schedule_call` instead

		if (!m_pSQVM || !m_pSQVM->sqvm)
		{
			spdlog::error(
				"{} was called on context {} while VM was not initialized. This will crash", __FUNCTION__, GetContextName(context));
		}

		SQObject functionobj {};
		int result = g_pSquirrel<context>->sq_getSquirrelFunction(g_pSquirrel<context>->m_pSQVM->sqvm, funcname, &functionobj, 0);
		if (result != 0) // This func returns 0 on success for some reason
		{
			return SQRESULT_ERROR;
		}
		g_pSquirrel<context>->pushSQObject(g_pSquirrel<context>->m_pSQVM->sqvm, &functionobj); // Push the function object
		g_pSquirrel<context>->pushroottable(g_pSquirrel<context>->m_pSQVM->sqvm); // Push root table
		return g_pSquirrel<context>->_call(g_pSquirrel<context>->m_pSQVM->sqvm, 0);
	}

	template <typename... Args> SQRESULT call(const char* funcname, Args... args)
	{
		// Warning!
		// This function assumes the squirrel VM is stopped/blocked at the moment of call
		// Calling this function while the VM is running is likely to result in a crash due to stack destruction
		// If you want to call into squirrel asynchronously, use `schedule_call` instead
		if (!m_pSQVM || !m_pSQVM->sqvm)
		{
			spdlog::error(
				"{} was called on context {} while VM was not initialized. This will crash", __FUNCTION__, GetContextName(context));
		}
		SQObject functionobj {};
		int result = g_pSquirrel<context>->sq_getSquirrelFunction(g_pSquirrel<context>->m_pSQVM->sqvm, funcname, &functionobj, 0);
		if (result != 0) // This func returns 0 on success for some reason
		{
			return SQRESULT_ERROR;
		}
		g_pSquirrel<context>->pushSQObject(g_pSquirrel<context>->m_pSQVM->sqvm, &functionobj); // Push the function object
		g_pSquirrel<context>->pushroottable(g_pSquirrel<context>->m_pSQVM->sqvm); // Push root table

		FunctionVector function_vector;
		SqRecurseArgs<context>(function_vector, args...);

		for (auto& v : function_vector)
		{
			// Execute lambda to push arg to stack
			v();
		}

		return g_pSquirrel<context>->_call(g_pSquirrel<context>->m_pSQVM->sqvm, function_vector.size());
	}

#pragma endregion

  public:
	SquirrelManager()
	{
		m_pSQVM = nullptr;
	}

	void VMCreated(CSquirrelVM* newSqvm);
	void VMDestroyed();
	void ExecuteCode(const char* code);
	void AddFuncRegistration(std::string returnType, std::string name, std::string argTypes, std::string helpText, SQFunction func);
	SQRESULT setupfunc(const SQChar* funcname);
	void AddFuncOverride(std::string name, SQFunction func);
};

template <ScriptContext context> SquirrelManager<context>* g_pSquirrel;

/*
	Beware all ye who enter below.
	This place is not a place of honor... no highly esteemed deed is commemorated here... nothing valued is here.
	What is here was dangerous and repulsive to us. This message is a warning about danger.
*/

#pragma region MessageBuffer templates

// Clang-formatting makes this whole thing unreadable
// clang-format off

#ifndef MessageBufferFuncs
#define MessageBufferFuncs
// Bools
template <ScriptContext context, typename T>
requires std::convertible_to<T, bool> && (!std::is_floating_point_v<T>) && (!std::convertible_to<T, std::string>) && (!std::convertible_to<T, int>)
inline VoidFunction SQMessageBufferPushArg(T& arg) {
	return [arg]{ g_pSquirrel<context>->pushbool(g_pSquirrel<context>->m_pSQVM->sqvm, static_cast<bool>(arg)); };
}
// Vectors
template <ScriptContext context>
inline VoidFunction SQMessageBufferPushArg(Vector3& arg) {
	return [arg]{ g_pSquirrel<context>->pushvector(g_pSquirrel<context>->m_pSQVM->sqvm, arg); };
}
// Vectors
template <ScriptContext context>
inline VoidFunction SQMessageBufferPushArg(SQObject* arg) {
	return [arg]{ g_pSquirrel<context>->pushSQObject(g_pSquirrel<context>->m_pSQVM->sqvm, arg); };
}
// Ints
template <ScriptContext context, typename T>
requires std::convertible_to<T, int> && (!std::is_floating_point_v<T>)
inline VoidFunction SQMessageBufferPushArg(T& arg) {
	return [arg]{ g_pSquirrel<context>->pushinteger(g_pSquirrel<context>->m_pSQVM->sqvm, static_cast<int>(arg)); };
}
// Floats
template <ScriptContext context, typename T>
requires std::convertible_to<T, float> && (std::is_floating_point_v<T>)
inline VoidFunction SQMessageBufferPushArg(T& arg) {
	return [arg]{ g_pSquirrel<context>->pushfloat(g_pSquirrel<context>->m_pSQVM->sqvm, static_cast<float>(arg)); };
}
// Strings
template <ScriptContext context, typename T>
requires (std::convertible_to<T, std::string> || std::is_constructible_v<std::string, T>)
inline VoidFunction SQMessageBufferPushArg(T& arg) {
	auto converted = std::string(arg);
	return [converted]{ g_pSquirrel<context>->pushstring(g_pSquirrel<context>->m_pSQVM->sqvm, converted.c_str(), converted.length()); };
}
// Assets
template <ScriptContext context>
inline VoidFunction SQMessageBufferPushArg(SquirrelAsset& arg) {
	return [arg]{ g_pSquirrel<context>->pushasset(g_pSquirrel<context>->m_pSQVM->sqvm, arg.path.c_str(), arg.path.length()); };
}
// Maps
template <ScriptContext context, typename T>
requires is_iterable<T>
inline VoidFunction SQMessageBufferPushArg(T& arg) {
	FunctionVector localv = {};
	localv.push_back([]{g_pSquirrel<context>->newarray(g_pSquirrel<context>->m_pSQVM->sqvm, 0);});
	
	for (const auto& item : arg) {
		localv.push_back(SQMessageBufferPushArg<context>(item));
		localv.push_back([]{g_pSquirrel<context>->arrayappend(g_pSquirrel<context>->m_pSQVM->sqvm, -2);});
	}

	return [localv] { for (auto& func : localv) { func(); } };
}
// Vectors
template <ScriptContext context, typename T>
requires is_map<T>
inline VoidFunction SQMessageBufferPushArg(T& map) {
	FunctionVector localv = {};
	localv.push_back([]{g_pSquirrel<context>->newtable(g_pSquirrel<context>->m_pSQVM->sqvm);});
	
	for (const auto& item : map) {
		localv.push_back(SQMessageBufferPushArg<context>(item.first));
		localv.push_back(SQMessageBufferPushArg<context>(item.second));
		localv.push_back([]{g_pSquirrel<context>->newslot(g_pSquirrel<context>->m_pSQVM->sqvm, -3, false);});
	}

	return [localv]{ for (auto& func : localv) { func(); } };
}

template <ScriptContext context, typename T>
inline void SqRecurseArgs(FunctionVector& v, T& arg) {
	v.push_back(SQMessageBufferPushArg<context>(arg));
}

// This function is separated from the PushArg function so as to not generate too many template instances
// This is the main function responsible for unrolling the argument pack
template <ScriptContext context, typename T, typename... Args>
inline void SqRecurseArgs(FunctionVector& v, T& arg, Args... args) {
	v.push_back(SQMessageBufferPushArg<context>(arg));
	SqRecurseArgs<context>(v, args...);
}

// clang-format on
#endif

#pragma endregion
