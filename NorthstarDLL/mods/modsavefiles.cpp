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
#include "scripts/scriptjson.h"

bool ContainsNonASCIIChars(std::string str)
{
	// we don't allow null characters either, even if they're ASCII characters because idk if people can
	// use it to circumvent the file extension suffix.
	return std::any_of(
		str.begin(), str.end(), [](char c) { return static_cast<unsigned char>(c) > 127 || static_cast<unsigned char>(c) == '\0'; });
}

ADD_SQFUNC("void", NSSaveFile, "string file, string data", "", ScriptContext::CLIENT | ScriptContext::UI | ScriptContext::SERVER)
{
	/* if (!saveFilesEnabled)
	{
		g_pSquirrel<context>->raiseerror(
			sqvm, fmt::format("Your mods have taken up too much space. Uninstall mods or reduce their character limits.").c_str());
		return SQRESULT_ERROR;
	}*/
	Mod* mod = g_pSquirrel<context>->getcallingmod(sqvm);
	if (mod == nullptr)
	{
		g_pSquirrel<context>->raiseerror(sqvm, "Has to be called from a mod function!");
		return SQRESULT_ERROR;
	}
	std::string fileName = g_pSquirrel<context>->getstring(sqvm, 1);
	std::string content = g_pSquirrel<context>->getstring(sqvm, 2);
	if (ContainsNonASCIIChars(content))
	{
		g_pSquirrel<context>->raiseerror(
			sqvm, fmt::format("File contents may not contain non-ASCII characters! Make sure your strings are valid!", mod->Name).c_str());
		return SQRESULT_ERROR;
	}
	for (ModSaveFile& file : mod->SaveFiles)
	{
		if (file.Name == fileName)
		{
			if (content.length() > file.CharacterLimit)
			{
				g_pSquirrel<context>->raiseerror(
					sqvm,
					fmt::format(
						"File content length over character limit ({})! Reduce the table's contents, or increase it!",
						file.CharacterLimit,
						mod->Name)
						.c_str());
				return SQRESULT_ERROR;
			}
			fs::create_directories(fs::path(GetNorthstarPrefix()) / "saveData" / fs::path(mod->m_ModDirectory).filename());
			std::ofstream fileStr(
				fs::path(GetNorthstarPrefix()) / "saveData" / fs::path(mod->m_ModDirectory).filename() / (fileName));
			if (fileStr.fail())
			{
				g_pSquirrel<context>->raiseerror(sqvm, fmt::format("There was an error opening/creating file {} (Is the file name valid?)", fileName).c_str());
				return SQRESULT_ERROR;
			}
			fileStr.write(content.c_str(), content.length());
			fileStr.close();
			return SQRESULT_NULL;
		}
	}
	g_pSquirrel<context>->raiseerror(sqvm, fmt::format("Mod with name {} was not found!", fileName).c_str());
	return SQRESULT_ERROR;
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

	if (!mod->m_bEnabled)
	{
		g_pSquirrel<context>->raiseerror(sqvm, fmt::format("Mod with name {} was not found!", mod->Name).c_str());
		return SQRESULT_ERROR;
	}
	for (ModSaveFile& file : mod->SaveFiles)
	{
		if (file.Name == fileName)
		{
			std::ifstream fileStr(
				fs::path(GetNorthstarPrefix()) / "saveData" / fs::path(mod->m_ModDirectory).filename() / (file.Name));
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
	}
	g_pSquirrel<context>->raiseerror(
		sqvm, fmt::format("File with name {} was not registered for mod {}!", fileName, mod->Name).c_str());
	return SQRESULT_ERROR;
}
