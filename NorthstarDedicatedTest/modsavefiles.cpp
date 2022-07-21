#include "pch.h"
#include <filesystem>
#include <sstream>
#include <fstream>
#include "configurables.h"
#include "squirrel.h"
#include "scriptjson.h"
#include "modmanager.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
const int CHARACTER_LIMIT = 8192;

bool ContainsNonASCIIChars(std::string str);
std::string EncodeJSON(void* sqvm);

SQRESULT ClientSq_SaveJSON(void* sqvm) 
{
	std::string modName = ClientSq_getstring(sqvm, 1);
	std::string file = ClientSq_getstring(sqvm, 2);
	std::string content = EncodeJSON(sqvm);
	if (content.length() > CHARACTER_LIMIT)
	{
		ClientSq_pusherror(
			sqvm,
			fmt::format(
				"File content length over character limit ({})! Reduce the table's contents, or use multiple files!",
				CHARACTER_LIMIT,
			modName).c_str());
		return SQRESULT_ERROR;
	}
	if (ContainsNonASCIIChars(content))
	{
		ClientSq_pusherror(sqvm, fmt::format("File contents may not contain non-ASCII characters! Make sure your strings are valid!",
			modName).c_str());
		return SQRESULT_ERROR;
	}
	for (Mod& mod : g_ModManager->m_loadedMods)
		if (modName == mod.Name)
		{
			if (!mod.Enabled)
			{
				ClientSq_pusherror(sqvm, fmt::format("Mod with name {} was not found!", modName).c_str());
				return SQRESULT_ERROR;
			}
			for (std::string fileName : mod.SaveFiles)
			{
				if (fileName == file)
				{
					fs::create_directories(fs::path(GetNorthstarPrefix()) / "saveData");
					std::ofstream fileStr(fs::path(GetNorthstarPrefix()) / "saveData" / (file + ".json"));
					if (fileStr.fail())
					{
						ClientSq_pusherror(sqvm, fmt::format("There was an error opening/creating file {} (Is the file name valid?)", file).c_str());
						return SQRESULT_ERROR;
					}
					fileStr.write(content.c_str(), content.length());
					fileStr.close();
					return SQRESULT_NULL;
				}
			}
		}
	ClientSq_pusherror(sqvm, fmt::format("Mod with name {} was not found!", file).c_str());
	return SQRESULT_ERROR;
}

SQRESULT ClientSq_LoadJSON(void* sqvm) 
{
	std::string modName = ClientSq_getstring(sqvm, 1);
	std::string file = ClientSq_getstring(sqvm, 2);
	for (Mod& mod : g_ModManager->m_loadedMods)
		if (modName == mod.Name)
		{
			if (!mod.Enabled)
			{
				ClientSq_pusherror(sqvm, fmt::format("Mod with name {} was not found!", modName).c_str());
				return SQRESULT_ERROR;
			}
			for (std::string fileName : mod.SaveFiles)
			{
				if (fileName == file)
				{
					std::ifstream fileStr(fs::path(GetNorthstarPrefix()) / "saveData" / (file + ".json"));
					if (fileStr.fail())
					{
						ClientSq_newTable(sqvm);
						return SQRESULT_NOTNULL;
					}
					std::stringstream jsonStringStream;
					while (fileStr.peek() != EOF)
						jsonStringStream << (char)fileStr.get();
					rapidjson_document doc;
					doc.Parse(jsonStringStream.str().c_str());
					if (doc.HasParseError())
					{
						ClientSq_newTable(sqvm);
						return SQRESULT_NOTNULL;
					}
					//spdlog::warn("Passing to DecodeJSON... {}", doc["test"].GetString());
					ClientSq_DecodeJSON_Table(
						sqvm, (rapidjson::GenericValue<rapidjson::UTF8<char>, rapidjson::MemoryPoolAllocator<SourceAllocator>>*)&doc);
					return SQRESULT_NOTNULL;
				}
			}
			ClientSq_pusherror(sqvm, fmt::format("File with name {} was not found!", file).c_str());
			return SQRESULT_ERROR;
		}
	ClientSq_pusherror(sqvm, fmt::format("Mod with name {} was not found!", modName).c_str());
	return SQRESULT_ERROR;
}

SQRESULT ServerSq_SaveJSON(void* sqvm)
{
	std::string modName = ServerSq_getstring(sqvm, 1);
	std::string file = ServerSq_getstring(sqvm, 2);
	std::string content = EncodeJSON(sqvm);
	if (content.length() > CHARACTER_LIMIT)
	{
		ServerSq_pusherror(
			sqvm,
			fmt::format(
				"File content length over character limit ({})! Reduce the table's contents, or use multiple files!",
				CHARACTER_LIMIT,
				modName)
				.c_str());
		return SQRESULT_ERROR;
	}
	if (ContainsNonASCIIChars(content))
	{
		ServerSq_pusherror(
			sqvm, fmt::format("File contents may not contain non-ASCII characters! Make sure your strings are valid!", modName).c_str());
		return SQRESULT_ERROR;
	}
	for (Mod& mod : g_ModManager->m_loadedMods)
		if (modName == mod.Name)
		{
			if (!mod.Enabled)
			{
				ServerSq_pusherror(sqvm, fmt::format("Mod with name {} was not found!", modName).c_str());
				return SQRESULT_ERROR;
			}
			for (std::string fileName : mod.SaveFiles)
			{
				if (fileName == file)
				{
					fs::create_directories(fs::path(GetNorthstarPrefix()) / "saveData");
					std::ofstream fileStr(fs::path(GetNorthstarPrefix()) / "saveData" / (file + ".json"));
					if (fileStr.fail())
					{
						ServerSq_pusherror(
							sqvm, fmt::format("There was an error opening/creating file {} (Is the file name valid?)", file).c_str());
						return SQRESULT_ERROR;
					}
					fileStr.write(content.c_str(), content.length());
					fileStr.close();
					return SQRESULT_NULL;
				}
			}
		}
	ServerSq_pusherror(sqvm, fmt::format("Mod with name {} was not found!", file).c_str());
	return SQRESULT_ERROR;
}

SQRESULT ServerSq_LoadJSON(void* sqvm)
{
	std::string modName = ServerSq_getstring(sqvm, 1);
	std::string file = ServerSq_getstring(sqvm, 2);
	for (Mod& mod : g_ModManager->m_loadedMods)
		if (modName == mod.Name)
		{
			if (!mod.Enabled)
			{
				ServerSq_pusherror(sqvm, fmt::format("Mod with name {} was not found!", modName).c_str());
				return SQRESULT_ERROR;
			}
			for (std::string fileName : mod.SaveFiles)
			{
				if (fileName == file)
				{
					std::ifstream fileStr(fs::path(GetNorthstarPrefix()) / "saveData" / (file + ".json"));
					if (fileStr.fail())
					{
						ServerSq_newTable(sqvm);
						return SQRESULT_NOTNULL;
					}
					std::stringstream jsonStringStream;
					while (fileStr.peek() != EOF)
						jsonStringStream << (char)fileStr.get();
					rapidjson_document doc;
					doc.Parse(jsonStringStream.str().c_str());
					if (doc.HasParseError())
					{
						ServerSq_newTable(sqvm);
						return SQRESULT_NOTNULL;
					}
					// spdlog::warn("Passing to DecodeJSON... {}", doc["test"].GetString());
					ServerSq_DecodeJSON_Table(
						sqvm, (rapidjson::GenericValue<rapidjson::UTF8<char>, rapidjson::MemoryPoolAllocator<SourceAllocator>>*)&doc);
					return SQRESULT_NOTNULL;
				}
			}
			ServerSq_pusherror(sqvm, fmt::format("File with name {} was not found!", file).c_str());
			return SQRESULT_ERROR;
		}
	ServerSq_pusherror(sqvm, fmt::format("Mod with name {} was not found!", modName).c_str());
	return SQRESULT_ERROR;
}

std::string EncodeJSON(void* sqvm) {
	rapidjson_document doc;
	doc.SetObject();

	HSquirrelVM* vm = (HSquirrelVM*)sqvm;
	SQTable* table = vm->_stackOfCurrentFunction[3]._VAL.asTable;
	SQ_EncodeJSON_Table(&doc, table, &doc);
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);
	return buffer.GetString();
}

void InitialiseClientSaveFiles(HMODULE baseAddress) 
{
	g_ClientSquirrelManager->AddFuncRegistration("table", "NSLoadFile", "string mod, string file", "", ClientSq_LoadJSON);
	g_ClientSquirrelManager->AddFuncRegistration("void", "NSSaveFile", "string mod, string file, table data", "", ClientSq_SaveJSON);

	g_UISquirrelManager->AddFuncRegistration("table", "NSLoadFile", "string mod, string file", "", ClientSq_LoadJSON);
	g_UISquirrelManager->AddFuncRegistration("void", "NSSaveFile", "string mod, string file, table data", "", ClientSq_SaveJSON);
}

void InitialiseServerSaveFiles(HMODULE baseAddress) 
{
	g_ServerSquirrelManager->AddFuncRegistration("table", "NSLoadFile", "string mod, string file", "", ServerSq_LoadJSON);
	g_ServerSquirrelManager->AddFuncRegistration("void", "NSSaveFile", "string mod, string file, table data", "", ServerSq_SaveJSON);
}

bool ContainsNonASCIIChars(std::string str) 
{
	// we don't allow null characters either, even if they're ASCII characters because idk if people can 
	// use it to circumvent the file extension suffix.
	return std::any_of(
		str.begin(), str.end(), [](char c) { return static_cast<unsigned char>(c) > 127 || static_cast<unsigned char>(c) == '\0'; });
}