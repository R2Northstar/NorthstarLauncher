#include "pch.h"
#include "squirrelnativeautobind.h"


std::vector<SqAutoBindFunc> clientSqAutoBindFuncs;
std::vector<SqAutoBindFunc> serverSqAutoBindFuncs;
AUTOHOOK_INIT();

ON_DLL_LOAD_RELIESON("client.dll", ClientSquirrelAutoBind, ClientSquirrel, (CModule module))
{
	
	for (auto& genFunc : clientSqAutoBindFuncs)
	{
		genFunc();
		
	}
}

ON_DLL_LOAD_RELIESON("server.dll", ServerSquirrelAutoBind, ServerSquirrel, (CModule module))
{
	for (auto& genFunc : serverSqAutoBindFuncs)
	{
		genFunc();
	}
}

