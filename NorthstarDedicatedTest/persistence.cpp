#include "pch.h"
#include "persistence.h"
#include "rapidjson/error/en.h"
#include "squirrel.h"
#include "gameutils.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include "configurables.h"
#include "rapidjson/document.h"
#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/writer.h"
// clang-format off
rapidjson_document persistenceDoc;

SQRESULT SQ_Persistence_Write(void* sqvm)
{
	std::string key = ServerSq_getstring(sqvm, 1);
	std::string value = ServerSq_getstring(sqvm, 2);
	
	if (!persistenceDoc.HasMember(key))
		persistenceDoc.AddMember(rapidjson::StringRef(key), rapidjson::StringRef(value), persistenceDoc.GetAllocator());
	else persistenceDoc[key].SetString(rapidjson::StringRef(value));

	return SQRESULT_NULL;
}

SQRESULT SQ_Persistence_Read(void* sqvm)
{
	std::string key = ServerSq_getstring(sqvm, 1);

	if (!persistenceDoc.HasMember(key) || !persistenceDoc[key].IsString())
		ServerSq_pusherror(sqvm, fmt::format("persistence.json doesn't contain key {}, or it isn't a valid string", key).c_str());
	else ServerSq_pushstring(sqvm, persistenceDoc[key].GetString(), -1);

	return SQRESULT_NOTNULL;
}

void InitializePersistence(HMODULE baseAddress) 
{
	g_ServerSquirrelManager->AddFuncRegistration("void", "NSPersistence_Write", "string key, string value", "", SQ_Persistence_Write);
	g_ServerSquirrelManager->AddFuncRegistration("string", "NSPersistence_Read", "string key", "", SQ_Persistence_Read);
	std::ifstream persistenceJson(GetNorthstarPrefix() + "/persistence.json");
	std::stringstream persistenceStringStream;

	if (!persistenceJson.fail())
	{
		persistenceStringStream << (char)persistenceJson.get();
		persistenceDoc.Parse<rapidjson::ParseFlag::kParseCommentsFlag | rapidjson::ParseFlag::kParseTrailingCommasFlag>(persistenceStringStream.str().c_str());
	}
	else persistenceDoc.SetObject();

	std::ofstream writeStream(GetNorthstarPrefix() + "/persistence.json");
	rapidjson::OStreamWrapper writeStreamWrapper(writeStream);
	rapidjson::Writer<rapidjson::OStreamWrapper> writer(writeStreamWrapper);

	persistenceDoc.Accept(writer);
}
// clang-format on