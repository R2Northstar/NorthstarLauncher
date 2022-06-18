#include "pch.h"
#include "NSMem.h"

#include "include/MinHook.h"

std::vector<int> NSMem::StringToHexBytes(const char* str)
{
	std::vector<int> patternNums;
	int size = strlen(str);
	for (int i = 0; i < size; i++)
	{
		char c = str[i];

		// If this is a space character, ignore it
		if (c == ' ' || c == '\t')
			continue;

		if (c == '?')
		{
			// Add a wildcard (-1)
			patternNums.push_back(-1);
		}
		else if (i < size - 1)
		{
			BYTE result = 0;
			for (int j = 0; j < 2; j++)
			{
				int val = 0;
				char c = *(str + i + j);
				if (c >= 'a')
				{
					val = c - 'a' + 0xA;
				}
				else if (c >= 'A')
				{
					val = c - 'A' + 0xA;
				}
				else if (isdigit(c))
				{
					val = c - '0';
				}
				else
				{
					assert(false, "Failed to parse invalid hex string.");
					val = -1;
				}

				result += (j == 0) ? val * 16 : val;
			}
			patternNums.push_back(result);
		}

		i++;
	}

	return patternNums;
}

// Tries its best to find potential start addresses to functions within a text section
// Not extremely accurate, but will catch 99% of functions, and the cache can give us a huge performance boost when pattern scanning!
void BuildFunctionAddressList(uintptr_t textSectionStart, size_t textSectionSize, std::vector<uintptr_t>& addressCacheOut)
{

	bool inFunction = false;

	for (uintptr_t curAddr = textSectionStart; curAddr < textSectionStart + textSectionSize; curAddr++)
	{
		BYTE b = *(BYTE*)curAddr;

		if (inFunction)
		{
			// Detect return instruction (there are others, but C3 is almost always used), see: https://www.felixcloutier.com/x86/ret
			// NOTE: Obviously a byte equal to C3 could appear in any instruction, but the majority of the time it's a function return
			if (b == 0xC3)
			{
				inFunction = false;
			}
		}
		else
		{
			if (b == 0xCC)
				continue; // Ignore padding between functions
			inFunction = true;
			addressCacheOut.push_back(curAddr);
		}
	}

	spdlog::debug(
		"NSMem: Built function address list lookup for .text at {} (list size: {})", (void*)textSectionStart, addressCacheOut.size());
}

void* NSMem::PatternScan(void* module, const int* pattern, int patternSize, int offset)
{
	assert(*pattern != -1, "First byte of pattern should never be a wildcard");

	auto dosHeader = (PIMAGE_DOS_HEADER)module;
	auto ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)module + dosHeader->e_lfanew);

	auto sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;
	auto scanBytes = (BYTE*)module;

	// This static map will store lists of assumed function start addresses for each module
	static std::unordered_map<void*, std::vector<uintptr_t>> moduleFuncAddrCacheMap;

	if (!moduleFuncAddrCacheMap.count(module)) // We don't have a cache for this module yet, make one!
	{
		moduleFuncAddrCacheMap[module] = std::vector<uintptr_t>();
		auto textSectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
		BuildFunctionAddressList(
			(uintptr_t)module + textSectionHeader->VirtualAddress, textSectionHeader->SizeOfRawData, moduleFuncAddrCacheMap[module]);
	}

	// Scan through our function address lookup list
	auto& funcAddrLookupList = moduleFuncAddrCacheMap[module];
	for (uintptr_t address : funcAddrLookupList)
	{
		BYTE* funcBytes = (BYTE*)address;

		bool found = true;
		for (auto i = 0; i < patternSize; i++)
		{
			// In theory, if you had a ridiculously large signature (tens of thousands of bytes),
			// you could eventually get this to read out of range - but that's obviously never going to happen
			if (funcBytes[i] != pattern[i] && pattern[i] != -1)
			{
				found = false;
				break;
			}
		}

		if (found)
			return (void*)address;
	}

	// Scan over every byte in the image after initial headers
	for (auto i = sizeof(IMAGE_DOS_HEADER) + sizeof(IMAGE_NT_HEADERS); i <= sizeOfImage - patternSize; i++)
	{
		bool found = true;
		for (auto j = 0; j < patternSize; j++)
		{
			if (scanBytes[i + j] != pattern[j] && pattern[j] != -1)
			{
				found = false;
				break;
			}
		}

		if (found)
			return scanBytes + i + offset;
	}

	return nullptr;
}

void* NSMem::PatternScan(const char* moduleName, const char* pattern, int offset)
{
	std::vector<int> patternNums = StringToHexBytes(pattern);
	assert(patternNums.size() > 0);

	uint64_t start = GetTickCount64();
	auto result = PatternScan(GetModuleHandleA(moduleName), &patternNums[0], patternNums.size(), offset);
	uint64_t timeTaken = GetTickCount64() - start;

	spdlog::debug("Found pattern in {}ms: \t\"{}\"", timeTaken, pattern);
	return result;
}

void NSMem::BytePatch(uintptr_t address, const BYTE* vals, int size)
{
	WriteProcessMemory(GetCurrentProcess(), (LPVOID)address, vals, size, NULL);
}

void NSMem::BytePatch(uintptr_t address, std::initializer_list<BYTE> vals)
{
	std::vector<BYTE> bytes = vals;
	if (!bytes.empty())
		BytePatch(address, &bytes[0], bytes.size());
}

void NSMem::BytePatch(uintptr_t address, const char* bytesStr)
{
	std::vector<int> byteInts = StringToHexBytes(bytesStr);
	std::vector<BYTE> bytes;
	for (int v : byteInts)
		bytes.push_back(v);

	if (!bytes.empty())
		BytePatch(address, &bytes[0], bytes.size());
}

void NSMem::NOP(uintptr_t address, int size)
{
	BYTE* buf = (BYTE*)malloc(size);
	memset(buf, 0x90, size);
	BytePatch(address, buf, size);
	free(buf);
}

bool NSMem::IsMemoryReadable(void* ptr, size_t size)
{
	static SYSTEM_INFO sysInfo;
	if (!sysInfo.dwPageSize)
		GetSystemInfo(&sysInfo); // This should always be 4096 unless ur playing on NES or some shit but whatever

	MEMORY_BASIC_INFORMATION memInfo;

	if (!VirtualQuery(ptr, &memInfo, sizeof(memInfo)))
		return false;

	if (memInfo.RegionSize < size)
		return false;

	return (memInfo.State & MEM_COMMIT) && !(memInfo.Protect & PAGE_NOACCESS);
}

bool KHook::Setup()
{
	if (targetFunc.IsPatternScan())
	{
		targetFuncAddr = NSMem::PatternScan(targetFunc.moduleName, targetFunc.pattern, targetFunc.offset);
	}
	else
	{
		targetFuncAddr = NSMem::GetOffsetPtr<void*>(GetModuleHandleA(targetFunc.moduleName), targetFunc.offset);
	}

	if (!targetFuncAddr)
		return false;

	return MH_CreateHook(targetFuncAddr, hookFunc, original) == MH_OK;
}

bool KHook::InitAllHooks()
{

	for (KHook* hook : _allHooks)
	{
		if (hook->Setup())
		{
			spdlog::debug("KHook hooked at {}", hook->targetFuncAddr);
		}
		else
		{
			return false;
		}
	}

	return MH_EnableHook(MH_ALL_HOOKS) == MH_OK;
}