#pragma once
#include <vector>

class __squirrelautobind;

typedef void (*SqAutoBindFunc)(void);
extern std::vector<SqAutoBindFunc> clientSqAutoBindFuncs;
extern std::vector<SqAutoBindFunc> serverSqAutoBindFuncs;


#define ADD_SQUIRREL_FUNC(returnType, funcName, argTypes, helpText, runOnContext)                                                          \
	template <ScriptContext context> SQRESULT CONCAT2(Script_, funcName)(HSquirrelVM * sqvm);                                              \
	namespace                                                                                                                              \
	{                                                                                                                                      \
		void CONCAT2(AUTOBIND_FUNC_CLIENT,funcName)()                                                                                                         \
		{   spdlog::info("TRYING TO AUTOBIND runOnContext = {} result = {}",runOnContext,runOnContext & ScriptContext::UI);                                                                                                                               \
			if constexpr (runOnContext & ScriptContext::UI)                                                                                \
			{                                                                                                                              \
				g_pSquirrel<ScriptContext::UI>->AddFuncRegistration(                                                                       \
					returnType, __STR(funcName), argTypes, helpText, CONCAT2(Script_, funcName) < ScriptContext::UI >);                    \
			}                                                                                                                              \
			if constexpr (runOnContext & ScriptContext::CLIENT)                                                                            \
			{                                                                                                                              \
				g_pSquirrel<ScriptContext::CLIENT>->AddFuncRegistration(                                                                   \
					returnType, __STR(funcName), argTypes, helpText, CONCAT2(Script_, funcName) < ScriptContext::CLIENT >);                \
			}                                                                                                                              \
		}                                                                                                                                  \
		void CONCAT2(AUTOBIND_FUNC_SERVER,funcName)()                                                                                                         \
		{                                                                                                                                  \
			if constexpr (runOnContext & ScriptContext::SERVER)                                                                            \
			{                                                                                                                              \
				g_pSquirrel<ScriptContext::SERVER>->AddFuncRegistration(                                                                   \
					returnType, __STR(funcName), argTypes, helpText, CONCAT2(Script_, funcName) < ScriptContext::SERVER >);                \
			}                                                                                                                              \
		}                                                                                                                                  \
		__squirrelautobind CONCAT2(__squirrelautobind, __LINE__)(CONCAT2(AUTOBIND_FUNC_CLIENT,funcName), CONCAT2(AUTOBIND_FUNC_SERVER,funcName));                                \
	}                                                                                                                                      \
	template <ScriptContext context> SQRESULT CONCAT2(Script_, funcName)(HSquirrelVM * sqvm)

#define REPLACE_SQUIRREL_FUNC(funcName, runOnContext)                                                                                      \
	template <ScriptContext context> SQRESULT CONCAT2(Script_, funcName)(HSquirrelVM * sqvm);                                              \
	namespace                                                                                                                              \
	{                                                                                                                                      \
		void CONCAT2(AUTOBIND_FUNC_CLIENT,funcName)()                                                                                                         \
		{                                                                                                                                  \
			if constexpr (runOnContext & ScriptContext::UI)                                                                                \
			{                                                                                                                              \
				g_pSquirrel<ScriptContext::UI>->AddFuncOverride(__STR(funcName), CONCAT2(Script_, funcName) < ScriptContext::UI >);        \
			}                                                                                                                              \
			if constexpr (runOnContext & ScriptContext::CLIENT)                                                                            \
			{                                                                                                                              \
				g_pSquirrel<ScriptContext::CLIENT>->AddFuncOverride(                                                                       \
					__STR(funcName), CONCAT2(Script_, funcName) < ScriptContext::CLIENT >);                                                \
			}                                                                                                                              \
		}                                                                                                                                  \
		void CONCAT2(AUTOBIND_FUNC_SERVER,funcName)()                                                                                                         \
		{                                                                                                                                  \
			if constexpr (runOnContext & ScriptContext::SERVER)                                                                            \
			{                                                                                                                              \
				g_pSquirrel<ScriptContext::SERVER>->AddFuncOverride(                                                                       \
					__STR(funcName), CONCAT2(Script_, funcName) < ScriptContext::SERVER >);                                                \
			}                                                                                                                              \
		}                                                                                                                                  \
		__squirrelautobind CONCAT2(__squirrelautobind, __LINE__)(CONCAT2(AUTOBIND_FUNC_CLIENT,funcName), CONCAT2(AUTOBIND_FUNC_SERVER,funcName));                                \
	}                                                                                                                                      \
	template <ScriptContext context> SQRESULT CONCAT2(Script_, funcName)(HSquirrelVM * sqvm)

class __squirrelautobind
{
  public:
	__squirrelautobind() = delete;

	__squirrelautobind(SqAutoBindFunc clientAutoBindFunc, SqAutoBindFunc serverAutoBindFunc)
	{
		clientSqAutoBindFuncs.push_back(clientAutoBindFunc);
		serverSqAutoBindFuncs.push_back(serverAutoBindFunc);
	}
};

