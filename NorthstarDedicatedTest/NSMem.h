#pragma once
#include "pch.h"

// KittenPopo's memory stuff, made for northstar (because I really can't handle working with northstar's original memory stuff tbh)

#pragma region Pattern Scanning
namespace NSMem
{
	std::vector<int> StringToHexBytes(const char* str);

	void* PatternScan(void* module, const int* pattern, int patternSize, int offset);

	void* PatternScan(const char* moduleName, const char* pattern, int offset = 0);

	void BytePatch(uintptr_t address, const BYTE* vals, int size);

	void BytePatch(uintptr_t address, std::initializer_list<BYTE> vals);

	void BytePatch(uintptr_t address, const char* bytesStr);

	void NOP(uintptr_t address, int size);

	bool IsMemoryReadable(void* ptr, size_t size);
} // namespace NSMem

#pragma region KHOOK
struct KHookPatternInfo
{
	const char *moduleName, *pattern;
	int offset = 0;

	KHookPatternInfo(const char* moduleName, const char* pattern, int offset = 0) : moduleName(moduleName), pattern(pattern), offset(offset)
	{
	}
};

struct KHook
{
	KHookPatternInfo targetFunc;
	void* targetFuncAddr;
	void* hookFunc;
	void** original;

	static inline std::vector<KHook*> _allHooks;

	KHook(KHookPatternInfo targetFunc, void* hookFunc, void** original) : targetFunc(targetFunc)
	{
		this->hookFunc = hookFunc;
		this->original = original;
		_allHooks.push_back(this);
	}

	bool Setup()
	{
		targetFuncAddr = NSMem::PatternScan(targetFunc.moduleName, targetFunc.pattern, targetFunc.offset);
		if (!targetFuncAddr)
			return false;

		return MH_CreateHook(targetFuncAddr, hookFunc, original) == MH_OK;
	}

	// Returns true if succeeded
	static bool InitAllHooks()
	{
		for (KHook* hook : _allHooks)
		{
			if (hook->Setup())
			{
				spdlog::info("KHook hooked at {}", hook->targetFuncAddr);
			}
			else
			{
				return false;
			}
		}

		return MH_EnableHook(MH_ALL_HOOKS) == MH_OK;
	}
};
#define KHOOK(name, funcPatternInfo, returnType, convention, args)                                                                         \
	returnType convention hk##name args;                                                                                                   \
	auto o##name = (returnType(convention*) args)0;                                                                                        \
	KHook k##name = KHook(KHookPatternInfo funcPatternInfo, &hk##name, (void**)&o##name);                                                  \
	returnType convention hk##name args
#pragma endregion