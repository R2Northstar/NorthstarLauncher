#include "pch.h"
#include <filesystem>
#include <sstream>
#include <fstream>
#include "squirrel/squirrel.h"
#include "util/utils.h"
#include "mods/modmanager.h"
#include "modsavefiles.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "config/profile.h"
#include "rapidjson/error/en.h"

SaveFileManager* g_saveFileManager;
const int MAX_FOLDER_SIZE = 52428800; // 50MB (50 * 1024 * 1024)
fs::path savePath;

template <ScriptContext context> void SaveFileManager::SaveFileAsync(fs::path file, std::string contents)
{
	auto m = std::ref(mutexMap[file]);
	std::thread writeThread(
		[m, file, contents]()
		{
			m.get().lock();

			std::ofstream fileStr(file);
			if (fileStr.fail())
			{
				spdlog::error("A file was supposed to be written to but we can't access it?!");
				m.get().unlock();
				return;
			}

			fileStr.write(contents.c_str(), contents.length());
			fileStr.close();

			m.get().unlock();
			// side-note: this causes a leak?
			// when a file is added to the map, it's never removed.
			// no idea how to fix this - because we have no way to check if there are other threads waiting to use this file(?)
			// tried to use m.try_lock(), but it's unreliable, it seems.
		});

	writeThread.detach();
}

template <ScriptContext context> int SaveFileManager::LoadFileAsync(fs::path file)
{
	int handle = ++m_iLastRequestHandle;
	auto m = std::ref(mutexMap[file]);
	std::thread readThread(
		[m, file, handle]()
		{
			m.get().lock();

			std::ifstream fileStr(file);
			if (fileStr.fail())
			{
				spdlog::error("A file was supposed to be loaded but we can't access it?!");
				// we should throw a script error here. But idk how lmao
				g_pSquirrel<context>->AsyncCall("NSHandleLoadResult", handle, "");
				m.get().unlock();
				return;
			}

			std::stringstream stringStream;
			while (fileStr.peek() != EOF)
				stringStream << (char)fileStr.get();

			g_pSquirrel<context>->AsyncCall("NSHandleLoadResult", handle, stringStream.str());

			m.get().unlock();
			// side-note: this causes a leak?
			// when a file is added to the map, it's never removed.
			// no idea how to fix this - because we have no way to check if there are other threads waiting to use this file(?)
			// tried to use m.try_lock(), but it's unreliable, it seems.
		});

	readThread.detach();
	return handle;
}

template <ScriptContext context> void SaveFileManager::DeleteFileAsync(fs::path file)
{
	// P.S. I don't like how we have to async delete calls but we do.
	auto m = std::ref(mutexMap[file]);
	std::thread deleteThread(
		[m, file]()
		{
			m.get().lock();

			fs::remove(file);

			m.get().unlock();
			// side-note: this causes a leak?
			// when a file is added to the map, it's never removed.
			// no idea how to fix this - because we have no way to check if there are other threads waiting to use this file(?)
			// tried to use m.try_lock(), but it's unreliable, it seems.
		});

	deleteThread.detach();
}

uintmax_t GetSizeOfFolderContentsMinusFile(fs::path dir, std::string file)
{
	uintmax_t result = 0;
	for (const auto& entry : fs::directory_iterator(dir))
	{
		if (entry.path().filename() == file)
			continue;
		result += fs::file_size(entry.path());
	}
	return result;
}
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

// void NSSaveFile( string file, string data )
ADD_SQFUNC("void", NSSaveFile, "string file, string data", "", ScriptContext::SERVER | ScriptContext::CLIENT | ScriptContext::UI)
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

	fs::path dir = savePath / fs::path(mod->m_ModDirectory).filename();
	fs::create_directories(dir);
	// this actually allows mods to go over the limit, but not by much
	// the limit is to prevent mods from taking gigabytes of space,
	// this ain't a cloud service.
	if (GetSizeOfFolderContentsMinusFile(dir, fileName) + content.length() > MAX_FOLDER_SIZE)
	{
		// tbh, you're either trying to fill the hard drive or use so much data, you SHOULD be congratulated.
		g_pSquirrel<context>->raiseerror(
			sqvm,
			fmt::format(
				"Congrats, {}!\nYou have reached the maximum folder size!\n\n... please delete something. Or make your file shorter. "
				"Either works.",
				mod->Name)
				.c_str());
		return SQRESULT_ERROR;
	}

	g_saveFileManager->SaveFileAsync<context>(dir / fileName, content);

	return SQRESULT_NULL;
}

#include "scripts/scriptjson.h"

// void NSSaveJSONFile(string file, table data)
ADD_SQFUNC("void", NSSaveJSONFile, "string file, table data", "", ScriptContext::SERVER | ScriptContext::CLIENT | ScriptContext::UI)
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

	fs::path dir = savePath / fs::path(mod->m_ModDirectory).filename();
	fs::create_directories(dir);
	// this actually allows mods to go over the limit, but not by much
	// the limit is to prevent mods from taking gigabytes of space,
	// this ain't a cloud service.
	if (GetSizeOfFolderContentsMinusFile(dir, fileName) + content.length() > MAX_FOLDER_SIZE)
	{
		// tbh, you're either trying to fill the hard drive or use so much data, you SHOULD be congratulated.
		g_pSquirrel<context>->raiseerror(
			sqvm,
			fmt::format(
				"Congrats, {}!\nYou have reached the maximum folder size!\n\n... please delete something. Or make your file shorter. "
				"Either works.",
				mod->Name)
				.c_str());
		return SQRESULT_ERROR;
	}

	g_saveFileManager->SaveFileAsync<context>(dir / fileName, content);

	return SQRESULT_NULL;
}

// int NS_InternalLoadFile(string file)
ADD_SQFUNC("int", NS_InternalLoadFile, "string file", "", ScriptContext::SERVER | ScriptContext::CLIENT | ScriptContext::UI)
{
	Mod* mod = g_pSquirrel<context>->getcallingmod(sqvm, 1); // the function that called NSLoadFile :)
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
	fs::path dir = savePath / fs::path(mod->m_ModDirectory).filename();

	g_pSquirrel<context>->pushinteger(sqvm, g_saveFileManager->LoadFileAsync<context>(dir / fileName));

	return SQRESULT_NOTNULL;
}

// bool NSDoesFileExist(string file)
ADD_SQFUNC("bool", NSDoesFileExist, "string file", "", ScriptContext::SERVER | ScriptContext::CLIENT | ScriptContext::UI)
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

	g_pSquirrel<context>->pushbool(sqvm, fs::exists(savePath / fs::path(mod->m_ModDirectory).filename() / (fileName)));
	return SQRESULT_NOTNULL;
}

// int NSGetFileSize(string file)
ADD_SQFUNC("int", NSGetFileSize, "string file", "", ScriptContext::SERVER | ScriptContext::CLIENT | ScriptContext::UI)
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
			(int)(fs::file_size(savePath / fs::path(mod->m_ModDirectory).filename() / fileName) / 1024)); // throws if file does not exist
	}
	catch (std::filesystem::filesystem_error const& ex)
	{
		spdlog::info(ex.what());
		g_pSquirrel<context>->pushinteger(sqvm, -1);
	}
	return SQRESULT_NOTNULL;
}

// void NSDeleteFile(string file)
ADD_SQFUNC("void", NSDeleteFile, "string file", "", ScriptContext::SERVER | ScriptContext::CLIENT | ScriptContext::UI)
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

	g_saveFileManager->DeleteFileAsync<context>(savePath / fs::path(mod->m_ModDirectory).filename() / fileName);
	return SQRESULT_NOTNULL;
}

ADD_SQFUNC("array<string>", NSGetAllFiles, "", "", ScriptContext::CLIENT | ScriptContext::UI | ScriptContext::SERVER)
{
	Mod* mod = g_pSquirrel<context>->getcallingmod(sqvm);
	for (const auto& entry : fs::directory_iterator(savePath / fs::path(mod->m_ModDirectory).filename()))
	{
		g_pSquirrel<context>->pushstring(sqvm, entry.path().filename().string().c_str());
		g_pSquirrel<context>->arrayappend(sqvm, -2);
	}
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

ON_DLL_LOAD("engine.dll", ModSaveFFiles_Init, (CModule module))
{
	savePath = fs::path(GetNorthstarPrefix()) / "save_data";
	g_saveFileManager = new SaveFileManager;
}
