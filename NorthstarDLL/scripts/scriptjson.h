#pragma once

#include "pch.h"
#include "rapidjson/error/en.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "squirrel/squirrel.h"

template <ScriptContext context> void EncodeJSONTable(
	SQTable* table,
	rapidjson::GenericValue<rapidjson::UTF8<char>, rapidjson::MemoryPoolAllocator<SourceAllocator>>* obj,
	rapidjson::MemoryPoolAllocator<SourceAllocator>& allocator);
