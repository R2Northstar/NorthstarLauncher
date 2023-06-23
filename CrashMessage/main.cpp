#include <windows.h>
#include <string>

//-----------------------------------------------------------------------------
// Purpose: Gets argument at index
//-----------------------------------------------------------------------------
const char* GetArg(int idx, int argc, char* argv[])
{
	if (idx >= argc)
		return "INVALID_INDEX";

	return argv[idx];
}

//-----------------------------------------------------------------------------
// Purpose: Entry point
//-----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	const char* pszPrefix = GetArg(1, argc, argv);
	const char* pszException = GetArg(2, argc, argv);
	const char* pszModule = GetArg(3, argc, argv);

	// Calculate size first
	int iSize =
		std::snprintf(NULL, 0, "Northstar has crashed! Crash info can be found at %s/logs!\n\n%s\n%s", pszPrefix, pszException, pszModule);

	// Print to buffer
	char* pszBuffer = new char[iSize + 1];
	std::snprintf(
		pszBuffer, iSize + 1, "Northstar has crashed! Crash info can be found at %s/logs!\n\n%s\n%s", pszPrefix, pszException, pszModule);

	MessageBoxA(NULL, pszBuffer, "Northstar Crashed", MB_ICONERROR | MB_OK);

	return 0;
}
