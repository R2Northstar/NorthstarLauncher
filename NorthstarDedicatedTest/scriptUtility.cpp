#include "pch.h"
#include "scriptUtility.h"
#include "squirrel.h"

SQRESULT ServerSq_StringToAsset(void* sqvm)
{
	const char* asset = ServerSq_getstring(sqvm, 1);
	ServerSq_pushAsset(sqvm, asset, -1);
	return SQRESULT_NOTNULL;
}

SQRESULT ClientSq_StringToAsset(void* sqvm)
{
	const char* asset = ServerSq_getstring(sqvm, 1);
	ClientSq_pushAsset(sqvm, asset, -1);
	return SQRESULT_NOTNULL;
}

void InitialiseClientSquirrelUtilityFunctions(HMODULE baseAddress)
{
	g_ClientSquirrelManager->AddFuncRegistration("asset", "StringToAsset", "string assetName", "", ClientSq_StringToAsset);
	g_UISquirrelManager->AddFuncRegistration("asset", "StringToAsset", "string assetName", "", ClientSq_StringToAsset);
}

void InitialiseServerSquirrelUtilityFunctions(HMODULE baseAddress)
{
	g_ServerSquirrelManager->AddFuncRegistration("asset", "StringToAsset", "string assetName", "", ServerSq_StringToAsset);
}