#include "pch.h"
#include <filesystem>
#include <sstream>
#include <fstream>
#include "squirrel/squirrel.h"
#include "util/utils.h"
#include "mods/modmanager.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "config/profile.h"
#include "rapidjson/error/en.h"

fs::path savePath = fs::path(GetNorthstarPrefix()) / "save_data";

bool ContainsNonASCIIChars(std::string str)
{
	// we don't allow null characters either, even if they're ASCII characters because idk if people can
	// use it to circumvent the file extension suffix.
	return std::any_of(
		str.begin(), str.end(), [](char c) { return static_cast<unsigned char>(c) > 127 || static_cast<unsigned char>(c) == '\0'; });
}
bool CheckFileName(std::string str)
{
	// we don't allow null characters either, even if they're ASCII characters because idk if people can
	// use it to circumvent the file extension suffix.
	return std::any_of(
		str.begin(),
		str.end(),
		[](char c)
		{
			return static_cast<unsigned char>(c) > 127 || static_cast<unsigned char>(c) == '\0' || static_cast<unsigned char>(c) == '\\' ||
				   static_cast<unsigned char>(c) == '//';
		});
}

ADD_SQFUNC("void", NSSaveFile, "string file, string data", "", ScriptContext::CLIENT | ScriptContext::UI | ScriptContext::SERVER)
{
	Mod* mod = g_pSquirrel<context>->getcallingmod(sqvm);
	if (mod == nullptr)
	{
		g_pSquirrel<context>->raiseerror(sqvm, "Has to be called from a mod function!");
		return SQRESULT_ERROR;
	}

	std::string fileName = g_pSquirrel<context>->getstring(sqvm, 1);
	if (CheckFileName(fileName))
	{
		g_pSquirrel<context>->raiseerror(
			sqvm,
			fmt::format("File name invalid ({})! Make sure it has no '\\', '/' or non-ASCII charcters!", fileName, mod->Name).c_str());
		return SQRESULT_ERROR;
	}

	std::string content = g_pSquirrel<context>->getstring(sqvm, 2);
	if (ContainsNonASCIIChars(content))
	{
		g_pSquirrel<context>->raiseerror(
			sqvm, fmt::format("File contents may not contain non-ASCII characters! Make sure your strings are valid!", mod->Name).c_str());
		return SQRESULT_ERROR;
	}

	fs::create_directories(savePath / fs::path(mod->m_ModDirectory).filename());
	std::ofstream fileStr(savePath / fs::path(mod->m_ModDirectory).filename() / (fileName));
	if (fileStr.fail())
	{
		g_pSquirrel<context>->raiseerror(
			sqvm, fmt::format("There was an error opening/creating file {} (Is the file name valid?)", fileName).c_str());
		return SQRESULT_ERROR;
	}
	fileStr.write(content.c_str(), content.length());
	fileStr.close();
	return SQRESULT_NULL;
}

#include "scripts/scriptjson.h"

ADD_SQFUNC("void", NSSaveJSONFile, "string file, table data", "", ScriptContext::CLIENT | ScriptContext::UI | ScriptContext::SERVER)
{
	Mod* mod = g_pSquirrel<context>->getcallingmod(sqvm);
	if (mod == nullptr)
	{
		g_pSquirrel<context>->raiseerror(sqvm, "Has to be called from a mod function!");
		return SQRESULT_ERROR;
	}

	std::string fileName = g_pSquirrel<context>->getstring(sqvm, 1);
	if (CheckFileName(fileName))
	{
		g_pSquirrel<context>->raiseerror(
			sqvm,
			fmt::format("File name invalid ({})! Make sure it has no '\\', '/' or non-ASCII charcters!", fileName, mod->Name).c_str());
		return SQRESULT_ERROR;
	}

	std::string content = EncodeJSON<context>(sqvm);
	if (ContainsNonASCIIChars(content))
	{
		g_pSquirrel<context>->raiseerror(
			sqvm, fmt::format("File contents may not contain non-ASCII characters! Make sure your strings are valid!", mod->Name).c_str());
		return SQRESULT_ERROR;
	}

	fs::create_directories(savePath / fs::path(mod->m_ModDirectory).filename());
	std::ofstream fileStr(savePath / fs::path(mod->m_ModDirectory).filename() / (fileName));
	if (fileStr.fail())
	{
		g_pSquirrel<context>->raiseerror(
			sqvm, fmt::format("There was an error opening/creating file {} (Is the file name valid?)", fileName).c_str());
		return SQRESULT_ERROR;
	}
	fileStr.write(content.c_str(), content.length());
	fileStr.close();

	return SQRESULT_NULL;
}

ADD_SQFUNC("string", NSLoadFile, "string file", "", ScriptContext::CLIENT | ScriptContext::UI | ScriptContext::SERVER)
{
	Mod* mod = g_pSquirrel<context>->getcallingmod(sqvm);
	if (mod == nullptr)
	{
		g_pSquirrel<context>->raiseerror(sqvm, "Has to be called from a mod function!");
		return SQRESULT_ERROR;
	}
	std::string fileName = g_pSquirrel<context>->getstring(sqvm, 1);
	if (CheckFileName(fileName))
	{
		g_pSquirrel<context>->raiseerror(
			sqvm,
			fmt::format("File name invalid ({})! Make sure it has no '\\', '/' or non-ASCII charcters!", fileName, mod->Name).c_str());
		return SQRESULT_ERROR;
	}

	std::ifstream fileStr(savePath / (fileName));
	if (fileStr.fail())
	{
		g_pSquirrel<context>->pushstring(sqvm, "");
		return SQRESULT_NOTNULL;
	}

	std::stringstream jsonStringStream;
	while (fileStr.peek() != EOF)
		jsonStringStream << (char)fileStr.get();
	g_pSquirrel<context>->pushstring(sqvm, jsonStringStream.str().c_str());

	return SQRESULT_NOTNULL;
}

ADD_SQFUNC("table", NSLoadJSONFile, "string file", "", ScriptContext::CLIENT | ScriptContext::UI | ScriptContext::SERVER)
{
	Mod* mod = g_pSquirrel<context>->getcallingmod(sqvm);
	if (mod == nullptr)
	{
		g_pSquirrel<context>->raiseerror(sqvm, "Has to be called from a mod function!");
		return SQRESULT_ERROR;
	}

	std::string fileName = g_pSquirrel<context>->getstring(sqvm, 1);
	if (CheckFileName(fileName))
	{
		g_pSquirrel<context>->raiseerror(
			sqvm,
			fmt::format("File name invalid ({})! Make sure it has no '\\', '/' or non-ASCII charcters!", fileName, mod->Name).c_str());
		return SQRESULT_ERROR;
	}

	std::ifstream fileStr(savePath / fs::path(mod->m_ModDirectory).filename() / (fileName));
	if (fileStr.fail())
	{
		g_pSquirrel<context>->pushstring(sqvm, "");
		return SQRESULT_NOTNULL;
	}

	std::stringstream jsonStringStream;
	while (fileStr.peek() != EOF)
		jsonStringStream << (char)fileStr.get();

	return DecodeJSON<context>(fileName, sqvm, jsonStringStream.str().c_str());
}

ADD_SQFUNC("bool", NSDoesFileExist, "string file", "", ScriptContext::CLIENT | ScriptContext::UI | ScriptContext::SERVER)
{
	Mod* mod = g_pSquirrel<context>->getcallingmod(sqvm);
	std::string fileName = g_pSquirrel<context>->getstring(sqvm, 1);
	if (CheckFileName(fileName))
	{
		g_pSquirrel<context>->raiseerror(
			sqvm,
			fmt::format("File name invalid ({})! Make sure it has no '\\', '/' or non-ASCII charcters!", fileName, mod->Name).c_str());
		return SQRESULT_ERROR;
	}

	std::ifstream fileStr(savePath / fs::path(mod->m_ModDirectory).filename() / (fileName));
	if (fileStr.fail())
	{
		g_pSquirrel<context>->pushbool(sqvm, false);
		return SQRESULT_NOTNULL;
	}

	g_pSquirrel<context>->pushbool(sqvm, true);
	return SQRESULT_NOTNULL;
}

ADD_SQFUNC("bool", NSGetFileSize, "string file", "", ScriptContext::CLIENT | ScriptContext::UI | ScriptContext::SERVER)
{
	Mod* mod = g_pSquirrel<context>->getcallingmod(sqvm);
	std::string fileName = g_pSquirrel<context>->getstring(sqvm, 1);
	if (CheckFileName(fileName))
	{
		g_pSquirrel<context>->raiseerror(
			sqvm,
			fmt::format("File name invalid ({})! Make sure it has no '\\', '/' or non-ASCII charcters!", fileName, mod->Name).c_str());
		return SQRESULT_ERROR;
	}
	try
	{
		g_pSquirrel<context>->pushinteger(
			sqvm,
			(int)(fs::file_size(savePath / fs::path(mod->m_ModDirectory).filename() / fileName) / 1000)); // throws if file does not exist
	}
	catch (std::filesystem::filesystem_error const& ex)
	{
		spdlog::info(ex.what());
		g_pSquirrel<context>->pushinteger(sqvm, -1);
	}
	return SQRESULT_NOTNULL;
}

ADD_SQFUNC("bool", NSDeleteFile, "string file", "", ScriptContext::CLIENT | ScriptContext::UI | ScriptContext::SERVER)
{
	Mod* mod = g_pSquirrel<context>->getcallingmod(sqvm);
	std::string fileName = g_pSquirrel<context>->getstring(sqvm, 1);
	if (CheckFileName(fileName))
	{
		g_pSquirrel<context>->raiseerror(
			sqvm,
			fmt::format("File name invalid ({})! Make sure it has no '\\', '/' or non-ASCII charcters!", fileName, mod->Name).c_str());
		return SQRESULT_ERROR;
	}

	g_pSquirrel<context>->pushbool(sqvm, fs::remove(savePath / fs::path(mod->m_ModDirectory).filename() / fileName));
	return SQRESULT_NOTNULL;
}

// ok, I'm just gonna explain what the fuck is going on here because this
// is the pinnacle of my stupidity and I do not want to touch this ever
// again & someone will eventually have to maintain this.

// P.S. if you don't want me to do the entire table -> JSON -> string thing...
// Fix it in scriptjson first k thx bye
template <ScriptContext context> std::string EncodeJSON(HSquirrelVM* sqvm)
{
	// new rapidjson
	rapidjson_document doc;
	doc.SetObject();

	HSquirrelVM* vm = (HSquirrelVM*)sqvm;

	// get the SECOND param
	SQTable* table = vm->_stackOfCurrentFunction[2]._VAL.asTable;
	// take the table and copy it's contents over into the rapidjson_document
	EncodeJSONTable<context>(table, &doc, doc.GetAllocator());

	// convert JSON document to string
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);

	// return the converted string
	return buffer.GetString();
}

template <ScriptContext context> SQRESULT DecodeJSON(std::string fileName, HSquirrelVM* sqvm, std::string content)
{
	rapidjson_document doc;
	doc.Parse(content);

	// basic parse checking shit
	if (doc.HasParseError())
	{
		g_pSquirrel<context>->newtable(sqvm);

		std::string sErrorString = fmt::format(
			"Failed parsing json file: encountered parse error for file {}:\n\n\"{}\" at offset {}",
			fileName,
			GetParseError_En(doc.GetParseError()),
			doc.GetErrorOffset());

		// errors are fatal.

		g_pSquirrel<context>->raiseerror(sqvm, sErrorString.c_str());
		return SQRESULT_ERROR;
	}

	// let scriptjson do the rest of the work since it automatically pushes the table to the stack
	DecodeJsonTable<context>(sqvm, (rapidjson::GenericValue<rapidjson::UTF8<char>, rapidjson::MemoryPoolAllocator<SourceAllocator>>*)&doc);
	return SQRESULT_NOTNULL;
}
