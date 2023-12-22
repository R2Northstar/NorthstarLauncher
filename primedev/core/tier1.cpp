#include "tier1.h"

// Note: this file is tier1/interface.cpp in primedev, but given that tier0 is yet to be split
// I am following the existing "pattern" and putting this here

CMemoryAddress Sys_GetFactoryPtr(const std::string& svModuleName, const std::string& svFactoryName)
{
	HMODULE hModule = GetModuleHandleA(svModuleName.c_str());

	if (!hModule)
		Error(eLog::NS, EXIT_FAILURE, "Failed to get module handle of '%s'!\n", svModuleName.c_str());

	CreateInterfaceFn fnCreateInterface = reinterpret_cast<CreateInterfaceFn>(GetProcAddress(hModule, CREATEINTERFACE_PROCNAME));

	return fnCreateInterface(svFactoryName.c_str(), NULL);
}
