#pragma once
#include <vector>

typedef void (*SqAutoBindFunc)(void);

class SquirrelAutoBindContainer
{
  public:
	std::vector<SqAutoBindFunc> clientSqAutoBindFuncs;
	std::vector<SqAutoBindFunc> serverSqAutoBindFuncs;
};

extern SquirrelAutoBindContainer* g_pSqAutoBindContainer;

class __squirrelautobind;

#define ADD_SQFUNC(returnType, funcName, argTypes, helpText, runOnContext)                                                                 \
	template <ScriptContext context> SQRESULT CONCAT2(Script_, funcName)(HSquirrelVM * sqvm);                                              \
	namespace                                                                                                                              \
	{                                                                                                                                      \
		void CONCAT2(AUTOBIND_FUNC_CLIENT, funcName)()                                                                                     \
		{                                                                                                                                  \
			if constexpr (runOnContext & ScriptContext::UI)                                                                                \
				g_pSquirrel<ScriptContext::UI>->AddFuncRegistration(                                                                       \
					returnType, __STR(funcName), argTypes, helpText, CONCAT2(Script_, funcName) < ScriptContext::UI >);                    \
                                                                                                                                           \
			if constexpr (runOnContext & ScriptContext::CLIENT)                                                                            \
				g_pSquirrel<ScriptContext::CLIENT>->AddFuncRegistration(                                                                   \
					returnType, __STR(funcName), argTypes, helpText, CONCAT2(Script_, funcName) < ScriptContext::CLIENT >);                \
		}                                                                                                                                  \
		void CONCAT2(AUTOBIND_FUNC_SERVER, funcName)()                                                                                     \
		{                                                                                                                                  \
			if constexpr (runOnContext & ScriptContext::SERVER)                                                                            \
				g_pSquirrel<ScriptContext::SERVER>->AddFuncRegistration(                                                                   \
					returnType, __STR(funcName), argTypes, helpText, CONCAT2(Script_, funcName) < ScriptContext::SERVER >);                \
		}                                                                                                                                  \
		__squirrelautobind                                                                                                                 \
			CONCAT2(__squirrelautobind, __LINE__)(CONCAT2(AUTOBIND_FUNC_CLIENT, funcName), CONCAT2(AUTOBIND_FUNC_SERVER, funcName));       \
	}                                                                                                                                      \
	template <ScriptContext context> SQRESULT CONCAT2(Script_, funcName)(HSquirrelVM * sqvm)

#define REPLACE_SQFUNC(funcName, runOnContext)                                                                                             \
	template <ScriptContext context> SQRESULT CONCAT2(Script_, funcName)(HSquirrelVM * sqvm);                                              \
	namespace                                                                                                                              \
	{                                                                                                                                      \
		void CONCAT2(AUTOBIND_FUNC_CLIENT, funcName)()                                                                                     \
		{                                                                                                                                  \
			if constexpr (runOnContext & ScriptContext::UI)                                                                                \
				g_pSquirrel<ScriptContext::UI>->AddFuncOverride(__STR(funcName), CONCAT2(Script_, funcName) < ScriptContext::UI >);        \
                                                                                                                                           \
			if constexpr (runOnContext & ScriptContext::CLIENT)                                                                            \
				g_pSquirrel<ScriptContext::CLIENT>->AddFuncOverride(                                                                       \
					__STR(funcName), CONCAT2(Script_, funcName) < ScriptContext::CLIENT >);                                                \
		}                                                                                                                                  \
		void CONCAT2(AUTOBIND_FUNC_SERVER, funcName)()                                                                                     \
		{                                                                                                                                  \
			if constexpr (runOnContext & ScriptContext::SERVER)                                                                            \
				g_pSquirrel<ScriptContext::SERVER>->AddFuncOverride(                                                                       \
					__STR(funcName), CONCAT2(Script_, funcName) < ScriptContext::SERVER >);                                                \
		}                                                                                                                                  \
		__squirrelautobind                                                                                                                 \
			CONCAT2(__squirrelautobind, __LINE__)(CONCAT2(AUTOBIND_FUNC_CLIENT, funcName), CONCAT2(AUTOBIND_FUNC_SERVER, funcName));       \
	}                                                                                                                                      \
	template <ScriptContext context> SQRESULT CONCAT2(Script_, funcName)(HSquirrelVM * sqvm)

class __squirrelautobind
{
  public:
	__squirrelautobind() = delete;

	__squirrelautobind(SqAutoBindFunc clientAutoBindFunc, SqAutoBindFunc serverAutoBindFunc)
	{
		// Bit hacky but we can't initialise this normally since this gets run automatically on load
		if (g_pSqAutoBindContainer == nullptr)
			g_pSqAutoBindContainer = new SquirrelAutoBindContainer();

		g_pSqAutoBindContainer->clientSqAutoBindFuncs.push_back(clientAutoBindFunc);
		g_pSqAutoBindContainer->serverSqAutoBindFuncs.push_back(serverAutoBindFunc);
	}
};
