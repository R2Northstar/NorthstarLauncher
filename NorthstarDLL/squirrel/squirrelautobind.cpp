#include "squirrelautobind.h"

SquirrelAutoBindContainer* g_pSqAutoBindContainer;

ON_DLL_LOAD_RELIESON("client.dll", ClientSquirrelAutoBind, ClientSquirrel, (CModule module))
{
	DevMsg(eLog::CLIENT, "ClientSquirrelAutoBInd AutoBindFuncsVectorsize %i\n", g_pSqAutoBindContainer->clientSqAutoBindFuncs.size());
	for (auto& autoBindFunc : g_pSqAutoBindContainer->clientSqAutoBindFuncs)
	{
		autoBindFunc();
	}
}

ON_DLL_LOAD_RELIESON("server.dll", ServerSquirrelAutoBind, ServerSquirrel, (CModule module))
{
	for (auto& autoBindFunc : g_pSqAutoBindContainer->serverSqAutoBindFuncs)
	{
		autoBindFunc();
	}
}
