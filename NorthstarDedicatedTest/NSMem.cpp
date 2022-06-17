#include "pch.h"
#include "NSMem.h"

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

void* NSMem::PatternScan(void* module, const int* pattern, int patternSize, int offset)
{
	if (!module)
		return NULL;

	auto dosHeader = (PIMAGE_DOS_HEADER)module;
	auto ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)module + dosHeader->e_lfanew);

	auto sizeOfImage = ntHeaders->OptionalHeader.SizeOfImage;

	auto scanBytes = (BYTE*)module;

	for (auto i = 0; i < sizeOfImage - patternSize; ++i)
	{
		bool found = true;
		for (auto j = 0; j < patternSize; ++j)
		{
			if (scanBytes[i + j] != pattern[j] && pattern[j] != -1)
			{
				found = false;
				break;
			}
		}

		if (found)
		{
			uintptr_t addressInt = (uintptr_t)(&scanBytes[i]) + offset;
			return (uint8_t*)addressInt;
		}
	}

	return nullptr;
}

void* NSMem::PatternScan(const char* moduleName, const char* pattern, int offset)
{
	std::vector<int> patternNums = StringToHexBytes(pattern);

	return PatternScan(GetModuleHandleA(moduleName), &patternNums[0], patternNums.size(), offset);
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
			spdlog::info("KHook hooked at {}", hook->targetFuncAddr);
		}
		else
		{
			return false;
		}
	}

	return MH_EnableHook(MH_ALL_HOOKS) == MH_OK;
}