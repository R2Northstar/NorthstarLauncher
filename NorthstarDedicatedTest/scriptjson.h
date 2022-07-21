#include "squirrel.h"
void InitialiseServerSquirrelJson(HMODULE baseAddress);
void InitialiseClientSquirrelJson(HMODULE baseAddress);
void SQ_EncodeJSON_Table(
	rapidjson::GenericValue<rapidjson::UTF8<char>, rapidjson::MemoryPoolAllocator<SourceAllocator>>* obj,
	SQTable* sqTable,
	rapidjson_document* allocatorDoc);
void SQ_EncodeJSON_Table(
	rapidjson::GenericValue<rapidjson::UTF8<char>, rapidjson::MemoryPoolAllocator<SourceAllocator>>* obj,
	SQTable* sqTable,
	rapidjson_document* allocatorDoc);
void ServerSq_DecodeJSON_Table(
	void* sqvm, rapidjson::GenericValue<rapidjson::UTF8<char>, rapidjson::MemoryPoolAllocator<SourceAllocator>>* obj);
void ClientSq_DecodeJSON_Table(
	void* sqvm, rapidjson::GenericValue<rapidjson::UTF8<char>, rapidjson::MemoryPoolAllocator<SourceAllocator>>* obj);
