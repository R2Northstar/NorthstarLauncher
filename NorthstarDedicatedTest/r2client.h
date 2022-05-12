#pragma once

// use the R2 namespace for game funcs
namespace R2
{
	extern char* g_LocalPlayerUserID;
	extern char* g_LocalPlayerOriginToken;
	
	typedef void* (*GetBaseLocalClientType)();
	extern GetBaseLocalClientType GetBaseLocalClient;
} // namespace R2