#pragma once

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

template <ScriptContext context> void EncodeJSONTable(
	SQTable* table,
	rapidjson::GenericValue<rapidjson::UTF8<char>, rapidjson::MemoryPoolAllocator<SourceAllocator>>* obj,
	rapidjson::MemoryPoolAllocator<SourceAllocator>& allocator);

template <ScriptContext context> void
DecodeJsonTable(HSQUIRRELVM sqvm, rapidjson::GenericValue<rapidjson::UTF8<char>, rapidjson::MemoryPoolAllocator<SourceAllocator>>* obj);
