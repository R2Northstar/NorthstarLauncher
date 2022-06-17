#pragma once
#include "pch.h"

// KittenPopo's memory stuff, made for northstar (because I really can't handle working with northstar's original memory stuff tbh)

#pragma region Pattern Scanning
namespace NSMem
{
	// Returns an array of byte values from a hexadecimal string
	// A byte value will be -1 where the input string contained a wildcard ('?')
	std::vector<int> StringToHexBytes(const char* str);

	/// <summary>
	/// Find a byte pattern in a module (raw function alternative)
	/// </summary>
	/// <param name="module">Base address of the module (handle)</param>
	/// <param name="pattern">Array of byte values to search for (-1 is for wildcard)</param>
	/// <param name="patternSize">Size of the byte pattern array</param>
	/// <param name="offset">Amount to shift output pointer address by</param>
	/// <returns>Address of found result (+ offset), or NULL if not found</returns>
	void* PatternScan(void* module, const int* pattern, int patternSize, int offset);

	/// <summary>
	/// Find a byte pattern in a module
	/// </summary>
	/// <param name="moduleName">Name of module, will be passed to GetModuleHandleA()</param>
	/// <param name="pattern">The byte pattern to search for as a hex string (see StringToHexBytes())</param>
	/// <param name="offset">Amount to shift output pointer address by</param>
	/// <returns>Address of found result (+ offset), or NULL if not found</returns>
	void* PatternScan(const char* moduleName, const char* pattern, int offset = 0);

	/// <summary>
	/// Byte patch memory at a given address
	/// </summary>
	/// <param name="address">Address to patch at</param>
	/// <param name="vals">Array of byte values to replace with</param>
	/// <param name="size">Size of byte value array</param>
	void BytePatch(uintptr_t address, const BYTE* vals, int size);

	/// <summary>
	/// Byte patch memory at a given address
	/// </summary>
	/// <param name="address">Address to patch at</param>
	/// <param name="vals">Initializer list of byte values to replace with</param>
	void BytePatch(uintptr_t address, std::initializer_list<BYTE> vals);

	/// <summary>
	/// Byte patch memory at a given address
	/// </summary>
	/// <param name="address">Address to patch at</param>
	/// <param name="bytesStr">Hex string of bytes to patch with (see StringToHexBytes())</param>
	void BytePatch(uintptr_t address, const char* bytesStr);

	/// <summary>
	/// Byte patch instructions at a given address to NOP opcodes, making them do nothing
	/// Functionally identical to using BytePatch(address, { 0x90, 0x90, 0x90, ... })
	/// </summary>
	/// <param name="address">Address to patch at</param>
	/// <param name="size">Amount of bytes to patch</param>
	void NOP(uintptr_t address, int size);

	/// <summary>
	/// Checks if we are able to read memory from a given address
	/// </summary>
	/// <param name="ptr">Address to check (start of region)</param>
	/// <param name="size">Size of the region to check</param>
	bool IsMemoryReadable(void* ptr, size_t size);

	/// <summary>
	/// Offset a pointer and convert it to a type
	/// </summary>
	/// <returns>(T)(uintptr_t(base) + offset)</returns>
	template <typename T> T GetOffsetPtr(void* base, int64_t offset)
	{
		return (T)(uintptr_t(base) + offset);
	}

	/// <summary>
	/// Offset a pointer and dereference it as a type
	/// </summary>
	/// <returns>*(T*)(uintptr_t(base) + offset)</returns>
	template <typename T> T& GetOffsetVal(void* base, int64_t offset)
	{
		return *(T*)(uintptr_t(base) + offset);
	}

} // namespace NSMem

#pragma region KHOOK
// Info for KHook to use when finding where you would like to hook
// Currently stores pattern scan information only
struct KHookPatternInfo
{
	const char *moduleName, *pattern;
	int offset = 0;

	KHookPatternInfo(const char* moduleName, const char* pattern, int offset = 0) : moduleName(moduleName), pattern(pattern), offset(offset)
	{
	}
};

// General structure for creating a hook (function trampoline detour) at any function
// Does not do anything until Setup() is called
struct KHook
{
	KHookPatternInfo targetFunc;
	void* targetFuncAddr;
	void* hookFunc;
	void** original;

	// NOTE: This is not thread-safe, perhaps a std::mutex lock should be used (?)
	static inline std::vector<KHook*> _allHooks;

	// NOTE: Will add this instance to KHook::_allHooks after construction
	KHook(KHookPatternInfo targetFunc, void* hookFunc, void** original) : targetFunc(targetFunc)
	{
		this->hookFunc = hookFunc;
		this->original = original;
		_allHooks.push_back(this);
	}

	// Actually create/enable the hook
	bool Setup();

	// Returns true if succeeded
	static bool InitAllHooks();
};

// Convenient macro for initializing a KHook as a function declaration in a single line
#define KHOOK(name, funcPatternInfo, returnType, convention, args)                                                                         \
	returnType convention hk##name args;                                                                                                   \
	auto o##name = (returnType(convention*) args)0;                                                                                        \
	KHook k##name = KHook(KHookPatternInfo funcPatternInfo, &hk##name, (void**)&o##name);                                                  \
	returnType convention hk##name args
#pragma endregion