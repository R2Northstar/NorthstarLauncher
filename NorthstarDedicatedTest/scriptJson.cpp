#include "pch.h"
#include "scriptJson.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#include "squirrel.h"

#ifdef _MSC_VER
#undef GetObject // fuck microsoft developers
#endif

void sq_EncodeJSON_Table(rapidjson::Value* obj, SQTable* sqTable, rapidjson::Document* allocatorDoc);
void sq_EncodeJSON_Array(rapidjson::Value* obj, SQArray* sqArray, rapidjson::Document* allocatorDoc);
void serverSq_DecodeJSON_Table(void* sqvm, rapidjson::Value* obj);
void serverSq_DecodeJSON_Array(void* sqvm, rapidjson::Value* arr);

SQRESULT serverSq_DecodeJSON(void* sqvm)
{
	const char* json = ServerSq_getstring(sqvm, 1);
	rapidjson::Document doc;
	doc.Parse(json);
	if (doc.HasParseError())
	{
		ServerSq_newTable(sqvm);
		return SQRESULT_NOTNULL;
	}
	ServerSq_newTable(sqvm);

	for (int i = 0; i < doc.MemberCount(); i++)
	{

		rapidjson::Value::Member& itr = doc.MemberBegin()[i];

		if ((long long)itr.name.GetString() == 0xcdcdcdcdcdcd)
			continue;

		switch (itr.value.GetType())
		{
		case rapidjson::kObjectType:
			ServerSq_pushstring(sqvm, itr.name.GetString(), -1);
			serverSq_DecodeJSON_Table(sqvm, (rapidjson::Value*)&itr.value);
			ServerSq_newSlot(sqvm, -3, false);
			break;
		case rapidjson::kArrayType:
			ServerSq_pushstring(sqvm, itr.name.GetString(), -1);
			serverSq_DecodeJSON_Array(sqvm, (rapidjson::Value*)&itr.value);
			ServerSq_newSlot(sqvm, -3, false);
			break;
		case rapidjson::kStringType:
			ServerSq_pushstring(sqvm, itr.name.GetString(), -1);
			ServerSq_pushstring(sqvm, itr.value.GetString(), -1);

			ServerSq_newSlot(sqvm, -3, false);
			break;
		case rapidjson::kTrueType:
			if ((long long)itr.name.GetString() == -1)
			{
				spdlog::info("Neagative String decoding True");
				continue;
			}

			ServerSq_pushstring(sqvm, itr.name.GetString(), -1);
			ServerSq_pushbool(sqvm, true);
			ServerSq_newSlot(sqvm, -3, false);
			break;
		case rapidjson::kFalseType:
			if ((long long)itr.name.GetString() == -1)
			{

				continue;
			}

			ServerSq_pushstring(sqvm, itr.name.GetString(), -1);
			ServerSq_pushbool(sqvm, false);
			ServerSq_newSlot(sqvm, -3, false);
			break;
		case rapidjson::kNumberType:
			if (itr.value.IsDouble() || itr.value.IsFloat())
			{

				ServerSq_pushstring(sqvm, itr.name.GetString(), -1);
				ServerSq_pushfloat(sqvm, itr.value.GetFloat());
			}
			else
			{
				ServerSq_pushstring(sqvm, itr.name.GetString(), -1);
				ServerSq_pushinteger(sqvm, itr.value.GetInt());
			}
			ServerSq_newSlot(sqvm, -3, false);
			break;
		}
	}
	return SQRESULT_NOTNULL;
}

void serverSq_DecodeJSON_Table(void* sqvm, rapidjson::Value* obj)
{
	ServerSq_newTable(sqvm);

	for (int i = 0; i < obj->MemberCount(); i++)
	{
		rapidjson::Value::Member& itr = obj->MemberBegin()[i];
		if (!itr.name.IsString())
		{
			spdlog::info("Decoding table with non-string key");
			continue;
		}
		const char* key = itr.name.GetString();
		if ((long long)key == 0xcdcdcdcdcdcd)
		{

			spdlog::info("Decoding table with invalid key string pointer");
			continue;
		}
		switch (itr.value.GetType())
		{
		case rapidjson::kObjectType:
			ServerSq_pushstring(sqvm, key, -1);
			serverSq_DecodeJSON_Table(sqvm, (rapidjson::Value*)&itr.value);
			ServerSq_newSlot(sqvm, -3, false);
			break;
		case rapidjson::kArrayType:
			ServerSq_pushstring(sqvm, key, -1);
			serverSq_DecodeJSON_Array(sqvm, (rapidjson::Value*)&itr.value);
			ServerSq_newSlot(sqvm, -3, false);
			break;
		case rapidjson::kStringType:
			if ((long long)key == 0xcdcdcdcdcdcd)
			{
				spdlog::info("Invalid String pointer in value with key {}", itr.name.GetString());
				continue;
			}
			ServerSq_pushstring(sqvm, key, -1);
			ServerSq_pushstring(sqvm, itr.value.GetString(), -1);
			ServerSq_newSlot(sqvm, -3, false);
			break;
		case rapidjson::kTrueType:

			ServerSq_pushstring(sqvm, key, -1);
			ServerSq_pushbool(sqvm, true);
			ServerSq_newSlot(sqvm, -3, false);
			break;
		case rapidjson::kFalseType:
			ServerSq_pushstring(sqvm, itr.name.GetString(), -1);
			ServerSq_pushbool(sqvm, false);
			ServerSq_newSlot(sqvm, -3, false);
			break;
		case rapidjson::kNumberType:
			if (itr.value.IsDouble() || itr.value.IsFloat())
			{
				ServerSq_pushstring(sqvm, itr.name.GetString(), -1);
				ServerSq_pushfloat(sqvm, itr.value.GetFloat());
			}
			else
			{
				ServerSq_pushstring(sqvm, itr.name.GetString(), -1);
				ServerSq_pushinteger(sqvm, itr.value.GetInt());
			}
			ServerSq_newSlot(sqvm, -3, false);
			break;
		}
	}
}

void serverSq_DecodeJSON_Array(void* sqvm, rapidjson::Value* arr)
{
	int usedType = arr->GetArray().Begin()->GetType();
	bool isFloat = arr->GetArray().Begin()->IsDouble() || arr->GetArray().Begin()->IsFloat();
	ServerSq_newarray(sqvm, 0);
	for (auto& itr : arr->GetArray())
	{
		switch (itr.GetType())
		{
		case rapidjson::kObjectType:

			if (usedType != itr.GetType())
				continue;
			serverSq_DecodeJSON_Table(sqvm, &itr);
			ServerSq_arrayappend(sqvm, -2);
			break;
		case rapidjson::kArrayType:

			if (usedType != itr.GetType())
				continue;
			serverSq_DecodeJSON_Array(sqvm, &itr);
			ServerSq_arrayappend(sqvm, -2);
			break;
		case rapidjson::kStringType:
			if ((long long)itr.GetString() == -1)
			{

				continue;
			}
			if (usedType != itr.GetType())
				continue;
			ServerSq_pushstring(sqvm, itr.GetString(), -1);
			ServerSq_arrayappend(sqvm, -2);
			break;
		case rapidjson::kTrueType:
			if (usedType != rapidjson::kTrueType && usedType != rapidjson::kFalseType)
				continue;
			ServerSq_pushbool(sqvm, true);
			ServerSq_arrayappend(sqvm, -2);
			break;
		case rapidjson::kFalseType:
			if (usedType != rapidjson::kTrueType && usedType != rapidjson::kFalseType)
				continue;
			ServerSq_pushbool(sqvm, false);
			ServerSq_arrayappend(sqvm, -2);
			break;
		case rapidjson::kNumberType:
			if (usedType != itr.GetType())
				continue;
			if (itr.IsDouble() || itr.IsFloat())
			{

				if (!isFloat)
					continue;
				ServerSq_pushfloat(sqvm, itr.GetFloat());
			}
			else
			{
				if (isFloat)
					continue;
				ServerSq_pushinteger(sqvm, itr.GetInt());
			}
			ServerSq_arrayappend(sqvm, -2);
			break;
		}
	}
}

SQRESULT sq_EncodeJSON(void* sqvm)
{
	rapidjson::Document doc;
	doc.SetObject();

	HSquirrelVM* vm = (HSquirrelVM*)sqvm;
	SQTable* table = vm->_stackOfCurrentFunction[1]._VAL.asTable;
	sq_EncodeJSON_Table(&doc, table, &doc);
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);
	const char* json = buffer.GetString();

	ServerSq_pushstring(sqvm, json, -1);
	return SQRESULT_NOTNULL;
}

void sq_EncodeJSON_Table(rapidjson::Value* obj, SQTable* table, rapidjson::Document* allocatorDoc)
{
	for (int i = 0; i < table->_numOfNodes; i++)
	{
		tableNode* node = &table->_nodes[i];
		if (node->key._Type == OT_STRING)
		{

			rapidjson::Value newObj(rapidjson::kObjectType);
			rapidjson::Value newArray(rapidjson::kArrayType);

			switch (node->val._Type)
			{
			case OT_STRING:

				obj->AddMember(
					rapidjson::StringRef(node->key._VAL.asString->_val),
					rapidjson::StringRef(node->val._VAL.asString->_val),
					allocatorDoc->GetAllocator());
				break;
			case OT_INTEGER:
				obj->AddMember(
					rapidjson::StringRef(node->key._VAL.asString->_val),
					rapidjson::Value((int)node->val._VAL.asInteger),
					allocatorDoc->GetAllocator());
				break;
			case OT_FLOAT:

				obj->AddMember(
					rapidjson::StringRef(node->key._VAL.asString->_val),
					rapidjson::Value(node->val._VAL.asFloat),
					allocatorDoc->GetAllocator());
				break;
			case OT_BOOL:
				if (node->val._VAL.asInteger)
				{
					obj->AddMember(
						rapidjson::StringRef(node->key._VAL.asString->_val), rapidjson::Value(true), allocatorDoc->GetAllocator());
				}
				else
				{
					obj->AddMember(
						rapidjson::StringRef(node->key._VAL.asString->_val), rapidjson::Value(false), allocatorDoc->GetAllocator());
				}
				break;
			case OT_TABLE:

				sq_EncodeJSON_Table(&newObj, node->val._VAL.asTable, allocatorDoc);
				obj->AddMember(rapidjson::StringRef(node->key._VAL.asString->_val), newObj, allocatorDoc->GetAllocator());
				break;
			case OT_ARRAY:

				sq_EncodeJSON_Array(&newArray, node->val._VAL.asArray, allocatorDoc);
				obj->AddMember(rapidjson::StringRef(node->key._VAL.asString->_val), newArray, allocatorDoc->GetAllocator());
				break;
			default:
				spdlog::info("SQ encode Json type {} not supported", sq_getTypeName(node->val._Type));
				break;
			}
		}
	}
}

void sq_EncodeJSON_Array(rapidjson::Value* obj, SQArray* sqArray, rapidjson::Document* allocatorDoc)
{
	int usedType = sqArray->_values->_Type;
	for (int i = 0; i < sqArray->_usedSlots; i++)
	{
		SQObject* node = &sqArray->_values[i];
		if (node->_Type != usedType)
		{
			const char* typeName = sq_getTypeName(node->_Type);
			const char* usedTypeName = sq_getTypeName(usedType);
			spdlog::info("SQ encode Json array not same type got %s expected %s", typeName, usedTypeName);
			continue;
		}
		rapidjson::Value newObj(rapidjson::kObjectType);
		rapidjson::Value newArray(rapidjson::kArrayType);
		switch (node->_Type)
		{
		case OT_STRING:
			obj->PushBack(rapidjson::StringRef(node->_VAL.asString->_val), allocatorDoc->GetAllocator());
			break;
		case OT_INTEGER:
			obj->PushBack(rapidjson::Value((int)node->_VAL.asInteger), allocatorDoc->GetAllocator());
			break;
		case OT_FLOAT:
			obj->PushBack(rapidjson::Value(node->_VAL.asFloat), allocatorDoc->GetAllocator());
			break;
		case OT_BOOL:
			if (node->_VAL.asInteger)
			{
				obj->PushBack(rapidjson::StringRef("true"), allocatorDoc->GetAllocator());
			}
			else
			{
				obj->PushBack(rapidjson::StringRef("false"), allocatorDoc->GetAllocator());
			}
			break;
		case OT_TABLE:

			sq_EncodeJSON_Table(&newObj, node->_VAL.asTable, allocatorDoc);
			obj->PushBack(newObj, allocatorDoc->GetAllocator());
			break;
		case OT_ARRAY:

			sq_EncodeJSON_Array(&newArray, node->_VAL.asArray, allocatorDoc);
			obj->PushBack(newArray, allocatorDoc->GetAllocator());
			break;
		default:

			spdlog::info("SQ encode Json type {} not supported", sq_getTypeName(node->_Type));
		}
	}
}

void clientSq_DecodeJSON_Table(void* sqvm, rapidjson::Value* obj);
void clientSq_DecodeJSON_Array(void* sqvm, rapidjson::Value* arr);

SQRESULT clientSq_DecodeJSON(void* sqvm)
{
	const char* json = ClientSq_getstring(sqvm, 1);
	rapidjson::Document doc;
	doc.Parse(json);
	if (doc.HasParseError())
	{
		ClientSq_newTable(sqvm);
		return SQRESULT_NOTNULL;
	}
	ServerSq_newTable(sqvm);

	for (int i = 0; i < doc.MemberCount(); i++)
	{

		rapidjson::Value::Member& itr = doc.MemberBegin()[i];

		if ((long long)itr.name.GetString() == 0xcdcdcdcdcdcd)
			continue;

		switch (itr.value.GetType())
		{
		case rapidjson::kObjectType:
			ClientSq_pushstring(sqvm, itr.name.GetString(), -1);
			clientSq_DecodeJSON_Table(sqvm, (rapidjson::Value*)&itr.value);
			ClientSq_newSlot(sqvm, -3, false);
			break;
		case rapidjson::kArrayType:
			ClientSq_pushstring(sqvm, itr.name.GetString(), -1);
			clientSq_DecodeJSON_Array(sqvm, (rapidjson::Value*)&itr.value);
			ClientSq_newSlot(sqvm, -3, false);
			break;
		case rapidjson::kStringType:
			ClientSq_pushstring(sqvm, itr.name.GetString(), -1);
			ClientSq_pushstring(sqvm, itr.value.GetString(), -1);

			ClientSq_newSlot(sqvm, -3, false);
			break;
		case rapidjson::kTrueType:
			if ((long long)itr.name.GetString() == -1)
			{
				spdlog::info("Neagative String decoding True");
				continue;
			}

			ClientSq_pushstring(sqvm, itr.name.GetString(), -1);
			ClientSq_pushbool(sqvm, true);
			ClientSq_newSlot(sqvm, -3, false);
			break;
		case rapidjson::kFalseType:
			if ((long long)itr.name.GetString() == -1)
			{

				continue;
			}

			ClientSq_pushstring(sqvm, itr.name.GetString(), -1);
			ClientSq_pushbool(sqvm, false);
			ClientSq_newSlot(sqvm, -3, false);
			break;
		case rapidjson::kNumberType:
			if (itr.value.IsDouble() || itr.value.IsFloat())
			{

				ClientSq_pushstring(sqvm, itr.name.GetString(), -1);
				ClientSq_pushfloat(sqvm, itr.value.GetFloat());
			}
			else
			{
				ClientSq_pushstring(sqvm, itr.name.GetString(), -1);
				ClientSq_pushinteger(sqvm, itr.value.GetInt());
			}
			ClientSq_newSlot(sqvm, -3, false);
			break;
		}
	}
	return SQRESULT_NOTNULL;
}

void clientSq_DecodeJSON_Table(void* sqvm, rapidjson::Value* obj)
{
	ClientSq_newTable(sqvm);

	for (int i = 0; i < obj->MemberCount(); i++)
	{
		rapidjson::Value::Member& itr = obj->MemberBegin()[i];
		if (!itr.name.IsString())
		{
			spdlog::info("Decoding table with non-string key");
			continue;
		}
		const char* key = itr.name.GetString();
		if ((long long)key == 0xcdcdcdcdcdcd)
		{

			spdlog::info("Decoding table with invalid key string pointer");
			continue;
		}
		switch (itr.value.GetType())
		{
		case rapidjson::kObjectType:
			ClientSq_pushstring(sqvm, key, -1);
			clientSq_DecodeJSON_Table(sqvm, (rapidjson::Value*)&itr.value);
			ClientSq_newSlot(sqvm, -3, false);
			break;
		case rapidjson::kArrayType:
			ClientSq_pushstring(sqvm, key, -1);
			clientSq_DecodeJSON_Array(sqvm, (rapidjson::Value*)&itr.value);
			ClientSq_newSlot(sqvm, -3, false);
			break;
		case rapidjson::kStringType:
			if ((long long)key == 0xcdcdcdcdcdcd)
			{
				spdlog::info("Invalid String pointer in value with key {}", itr.name.GetString());
				continue;
			}
			ClientSq_pushstring(sqvm, key, -1);
			ClientSq_pushstring(sqvm, itr.value.GetString(), -1);
			ClientSq_newSlot(sqvm, -3, false);
			break;
		case rapidjson::kTrueType:

			ClientSq_pushstring(sqvm, key, -1);
			ClientSq_pushbool(sqvm, true);
			ClientSq_newSlot(sqvm, -3, false);
			break;
		case rapidjson::kFalseType:
			ClientSq_pushstring(sqvm, itr.name.GetString(), -1);
			ClientSq_pushbool(sqvm, false);
			ClientSq_newSlot(sqvm, -3, false);
			break;
		case rapidjson::kNumberType:
			if (itr.value.IsDouble() || itr.value.IsFloat())
			{
				ClientSq_pushstring(sqvm, itr.name.GetString(), -1);
				ClientSq_pushfloat(sqvm, itr.value.GetFloat());
			}
			else
			{
				ClientSq_pushstring(sqvm, itr.name.GetString(), -1);
				ClientSq_pushinteger(sqvm, itr.value.GetInt());
			}
			ClientSq_newSlot(sqvm, -3, false);
			break;
		}
	}
}

void clientSq_DecodeJSON_Array(void* sqvm, rapidjson::Value* arr)
{
	int usedType = arr->GetArray().Begin()->GetType();
	bool isFloat = arr->GetArray().Begin()->IsDouble() || arr->GetArray().Begin()->IsFloat();
	ClientSq_newarray(sqvm, 0);
	for (auto& itr : arr->GetArray())
	{
		switch (itr.GetType())
		{
		case rapidjson::kObjectType:

			if (usedType != itr.GetType())
				continue;
			clientSq_DecodeJSON_Table(sqvm, &itr);
			ClientSq_arrayappend(sqvm, -2);
			break;
		case rapidjson::kArrayType:

			if (usedType != itr.GetType())
				continue;
			clientSq_DecodeJSON_Array(sqvm, &itr);
			ClientSq_arrayappend(sqvm, -2);
			break;
		case rapidjson::kStringType:
			if ((long long)itr.GetString() == -1)
			{

				continue;
			}
			if (usedType != itr.GetType())
				continue;
			ClientSq_pushstring(sqvm, itr.GetString(), -1);
			ClientSq_arrayappend(sqvm, -2);
			break;
		case rapidjson::kTrueType:
			if (usedType != rapidjson::kTrueType && usedType != rapidjson::kFalseType)
				continue;
			ClientSq_pushbool(sqvm, true);
			ClientSq_arrayappend(sqvm, -2);
			break;
		case rapidjson::kFalseType:
			if (usedType != rapidjson::kTrueType && usedType != rapidjson::kFalseType)
				continue;
			ClientSq_pushbool(sqvm, false);
			ClientSq_arrayappend(sqvm, -2);
			break;
		case rapidjson::kNumberType:
			if (usedType != itr.GetType())
				continue;
			if (itr.IsDouble() || itr.IsFloat())
			{

				if (!isFloat)
					continue;
				ClientSq_pushfloat(sqvm, itr.GetFloat());
			}
			else
			{
				if (isFloat)
					continue;
				ClientSq_pushinteger(sqvm, itr.GetInt());
			}
			ClientSq_arrayappend(sqvm, -2);
			break;
		}
	}
}

SQRESULT serverSq_castToTable(void* sqvm)
{
	HSquirrelVM* vm = (HSquirrelVM*)sqvm;
	SQObject* table = &vm->_stackOfCurrentFunction[1];
	SQObject* target = &vm->_stackOfCurrentFunction[2];
	if (table->_Type != OT_TABLE)
	{
		spdlog::info("Casting to table failed");
		ServerSq_newTable(sqvm);
		return SQRESULT_NOTNULL;
	}
	DECREMENT_REFERENCECOUNT(target);
	target->_Type = OT_TABLE;
	target->_VAL = table->_VAL;
	return SQRESULT_NOTNULL;
}

SQRESULT clientSq_castToTable(void* sqvm)
{
	HSquirrelVM* vm = (HSquirrelVM*)sqvm;
	SQObject* table = &vm->_stackOfCurrentFunction[1];
	SQObject* target = &vm->_stackOfCurrentFunction[2];
	if (table->_Type != OT_TABLE)
	{
		spdlog::info("Casting to table failed");
		ClientSq_newTable(sqvm);
		return SQRESULT_NOTNULL;
	}
	DECREMENT_REFERENCECOUNT(target);
	target->_Type = OT_TABLE;
	target->_VAL = table->_VAL;
	return SQRESULT_NOTNULL;
}

SQRESULT serverSq_castToArray(void* sqvm)
{
	HSquirrelVM* vm = (HSquirrelVM*)sqvm;
	SQObject* arr = &vm->_stackOfCurrentFunction[1];
	SQObject* target = &vm->_stackOfCurrentFunction[2];
	if (arr->_Type != OT_ARRAY)
	{
		spdlog::info("Casting to array failed");
		return SQRESULT_NOTNULL;
	}
	DECREMENT_REFERENCECOUNT(target);
	target->_Type = OT_ARRAY;
	target->_VAL = arr->_VAL;
	return SQRESULT_NOTNULL;
}

SQRESULT clientSq_castToArray(void* sqvm)
{
	HSquirrelVM* vm = (HSquirrelVM*)sqvm;
	SQObject* arr = &vm->_stackOfCurrentFunction[1];
	SQObject* target = &vm->_stackOfCurrentFunction[2];
	if (arr->_Type != OT_ARRAY)
	{
		spdlog::info("Casting to array failed");
		ClientSq_newarray(sqvm, 0);
		return SQRESULT_NOTNULL;
	}
	DECREMENT_REFERENCECOUNT(target);
	target->_Type = OT_ARRAY;
	target->_VAL = arr->_VAL;
	return SQRESULT_NOTNULL;
}

void InitialiseSquirrelJson(HMODULE baseAddress)
{

	g_ServerSquirrelManager->AddFuncRegistration("table", "DecodeJSON", "string json", "", serverSq_DecodeJSON);
	g_ServerSquirrelManager->AddFuncRegistration("string", "EncodeJSON", "table data", "", sq_EncodeJSON);
	g_ServerSquirrelManager->AddFuncRegistration("table", "castToTable", "var tab", "", serverSq_castToTable);
	g_ServerSquirrelManager->AddFuncRegistration("array", "castToArray", "var arr", "", serverSq_castToArray);

	g_ClientSquirrelManager->AddFuncRegistration("table", "DecodeJSON", "string json", "", clientSq_DecodeJSON);
	g_ClientSquirrelManager->AddFuncRegistration("string", "EncodeJSON", "table data", "", sq_EncodeJSON);
	g_ClientSquirrelManager->AddFuncRegistration("table", "castToTable", "var table", "", clientSq_castToTable);
	g_ClientSquirrelManager->AddFuncRegistration("array", "castToArray", "var array", "", clientSq_castToArray);

	g_UISquirrelManager->AddFuncRegistration("table", "DecodeJSON", "string json", "", clientSq_DecodeJSON);
	g_UISquirrelManager->AddFuncRegistration("string", "EncodeJSON", "table data", "", sq_EncodeJSON);
	g_UISquirrelManager->AddFuncRegistration("table", "castToTable", "var table", "", clientSq_castToTable);
	g_UISquirrelManager->AddFuncRegistration("array", "castToArray", "var array", "", clientSq_castToArray);
}