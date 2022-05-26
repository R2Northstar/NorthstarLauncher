#include "pch.h"

#include "NSQApi.h"
#include "NSQIO.h"
#include "clientchathooks.h"
#include "squirrel.h"
#include "serverchathooks.h"
#include "localchatwriter.h"

struct SQFunc;
std::vector<SQFunc*> sqFuncs;
struct SQFunc
{
	SQFunction func;
	std::string retType, name, args, helpStr;

	SQFunc(SQFunction func, std::string retType, std::string name, std::string args, std::string helpStr)
	{
		this->func = func;
		this->retType = retType;
		this->name = name;
		this->args = args;
		this->helpStr = helpStr;
		sqFuncs.push_back(this);
	}

	~SQFunc()
	{
		assert(false); // Bad, do not
	}
};

#define SQFUNC(retType, name, args, helpStr)                                                                                               \
	SQRESULT fnSQ_##name(void* sqvm);                                                                                                      \
	const SQFunc _fnSQ_##name = SQFunc(fnSQ_##name, retType, #name, args, helpStr);                                                        \
	SQRESULT fnSQ_##name(void* sqvm)

#pragma region Squirrel func definitions

#pragma region Chat // Previously in InitialiseClientChatHooks
// void NSChatWrite( int context, string str )
SQFUNC("void", NSChatWrite, "int context, string str", "")
{
	int context = ClientSq_getinteger(sqvm, 1);
	const char* str = ClientSq_getstring(sqvm, 2);

	LocalChatWriter((LocalChatWriter::Context)context).Write(str);
	return SQRESULT_NOTNULL;
}

// void NSChatWriteRaw( int context, string str )
SQFUNC("void", NSChatWriteRaw, "int context, string str", "")
{
	int context = ClientSq_getinteger(sqvm, 1);
	const char* str = ClientSq_getstring(sqvm, 2);

	LocalChatWriter((LocalChatWriter::Context)context).InsertText(str);
	return SQRESULT_NOTNULL;
}

// void NSChatWriteLine( int context, string str )
SQFUNC("void", NSChatWriteLine, "int context, string str", "")
{
	int context = ClientSq_getinteger(sqvm, 1);
	const char* str = ClientSq_getstring(sqvm, 2);

	LocalChatWriter((LocalChatWriter::Context)context).WriteLine(str);
	return SQRESULT_NOTNULL;
}
#pragma endregion

#pragma region NSQIO

typedef uint32_t JSONID; // You are only allowed to load 4,294,967,296 json files (literally 1984)
std::map<JSONID, rapidjson_document> loadedJsons;

// clang-format off
#define TRY_GET_JSON(varName, id) \
auto __get_json_itr = loadedJsons.find(id); \
if (__get_json_itr == loadedJsons.end()) { ClientSq_pusherror(sqvm, "Tried to access JSON file with invalid ID"); return SQRESULT_NULL; }\
auto& varName = __get_json_itr->second;
// clang-format on

JSONID lastId = 0;

SQFUNC("int", NsMakeJson, "", "Make an empty JSON file (returns JSON ID)")
{
	JSONID id = ++lastId;
	loadedJsons[id] = rapidjson_document();

	ClientSq_pushinteger(sqvm, lastId);
	return SQRESULT_NOTNULL;
}

SQFUNC("int", NsLoadJson, "string path", "Load a JSON file (returns JSON ID)")
{
	const char* path = ClientSq_getstring(sqvm, 1);

	loadedJsons[++lastId] = NSQIO::LoadJSON(path);

	ClientSq_pushinteger(sqvm, lastId);
	return SQRESULT_NOTNULL;
}

SQFUNC("void", NsUnloadJson, "int id", "Unload a JSON file")
{
	JSONID id = ClientSq_getinteger(sqvm, 1);

	if (!loadedJsons.erase(id))
	{
		ClientSq_pusherror(sqvm, "Tried to unload JSON file with invalid ID");
	}
	return SQRESULT_NULL;
}

SQFUNC("void", NsSaveJson, "int id, string path", "Save a JSON file")
{
	JSONID id = ClientSq_getinteger(sqvm, 1);
	TRY_GET_JSON(doc, id);

	std::string path = ClientSq_getstring(sqvm, 2);
	if (!NSQIO::SaveJSON(path, doc))
	{
		ClientSq_pusherror(sqvm, ("Tried to save JSON to invalid path \"" + path + '"').c_str());
	}

	return SQRESULT_NULL;
}

SQFUNC("string", NsGetJsonStr, "int id, string memberName", "Get a member of a loaded JSON file")
{
	JSONID id = ClientSq_getinteger(sqvm, 1);
	std::string memberName = ClientSq_getstring(sqvm, 2);

	TRY_GET_JSON(doc, id);

	if (doc.HasMember(memberName))
	{
		ClientSq_pushstring(sqvm, doc[memberName].GetString(), -1);
	}
	else
	{
		ClientSq_pushstring(sqvm, "", -1);
	}

	return SQRESULT_NOTNULL;
}
SQFUNC("void", NsSetJsonStr, "int id, string memberName, string value", "Set a member of a loaded JSON file")
{
	JSONID id = ClientSq_getinteger(sqvm, 1);
	std::string memberName = ClientSq_getstring(sqvm, 2);
	std::string value = ClientSq_getstring(sqvm, 3);

	TRY_GET_JSON(doc, id);

	doc[memberName].SetString(value.data(), value.size(), doc.GetAllocator());

	return SQRESULT_NOTNULL;
}

SQFUNC(
	"int",
	NsGetJsonInt,
	"int id, string memberName, int defaultValue",
	"Get an int member of a loaded JSON file (if not valid, returns default value)")
{
	JSONID id = ClientSq_getinteger(sqvm, 1);
	std::string memberName = ClientSq_getstring(sqvm, 2);
	int defaultVal = ClientSq_getinteger(sqvm, 3);

	TRY_GET_JSON(doc, id);

	if (doc.HasMember(memberName))
	{
		auto& member = doc[memberName];
		ClientSq_pushinteger(sqvm, member.IsInt() ? member.GetInt() : defaultVal);
	}
	else
	{
		ClientSq_pushinteger(sqvm, defaultVal);
	}

	return SQRESULT_NOTNULL;
}

SQFUNC("void", NsSetJsonInt, "int id, string memberName, int val", "Set an int member of a loaded JSON file")
{
	JSONID id = ClientSq_getinteger(sqvm, 1);
	std::string memberName = ClientSq_getstring(sqvm, 2);
	int val = ClientSq_getinteger(sqvm, 3);

	TRY_GET_JSON(doc, id);

	doc[memberName] = val;

	return SQRESULT_NOTNULL;
}

#pragma endregion

void NSQApi::InitClient(HMODULE unused)
{
	spdlog::info("Initializing NSQApi...");
	for (auto sqFunc : sqFuncs)
		g_ClientSquirrelManager->AddFuncRegistration(sqFunc->retType, sqFunc->name, sqFunc->args, sqFunc->helpStr, sqFunc->func);

	spdlog::info("\tRegistered {} squirrel funcs.", sqFuncs.size());
}