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
#include "core/tier0.h"
#include "rapidjson/error/en.h"

SaveFileManager* g_saveFileManager;
int MAX_FOLDER_SIZE = 52428800; // 50MB (50 * 1024 * 1024)
fs::path savePath;

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

template <ScriptContext context> void SaveFileManager::SaveFileAsync(fs::path file, std::string contents)
{
	auto mutex = std::ref(mutexMap[file]);
	std::thread writeThread(
		[mutex, file, contents]()
		{
			try
			{
				mutex.get().lock();

				fs::path dir = file.parent_path();
				// this actually allows mods to go over the limit, but not by much
				// the limit is to prevent mods from taking gigabytes of space,
				// this ain't a cloud service.
				if (GetSizeOfFolderContentsMinusFile(dir, file.filename().string()) + contents.length() > MAX_FOLDER_SIZE)
				{
					// tbh, you're either trying to fill the hard drive or use so much data, you SHOULD be congratulated.
					spdlog::error(fmt::format("Mod spamming save requests? Folder limit bypassed despite previous checks. Not saving."));
					mutex.get().unlock();
					return;
				}

				std::ofstream fileStr(file);
				if (fileStr.fail())
				{
					mutex.get().unlock();
					return;
				}

				fileStr.write(contents.c_str(), contents.length());
				fileStr.close();

				mutex.get().unlock();
				// side-note: this causes a leak?
				// when a file is added to the map, it's never removed.
				// no idea how to fix this - because we have no way to check if there are other threads waiting to use this file(?)
				// tried to use m.try_lock(), but it's unreliable, it seems.
			}
			catch (std::exception ex)
			{
				spdlog::error("SAVE FAILED!");
				mutex.get().unlock();
				spdlog::error(ex.what());
			}
		});

	writeThread.detach();
}

template <ScriptContext context> int SaveFileManager::LoadFileAsync(fs::path file)
{
	int handle = ++m_iLastRequestHandle;
	auto mutex = std::ref(mutexMap[file]);
	std::thread readThread(
		[mutex, file, handle]()
		{
			try
			{
				mutex.get().lock();

				std::ifstream fileStr(file);
				if (fileStr.fail())
				{
					spdlog::error("A file was supposed to be loaded but we can't access it?!");
					// we should throw a script error here. But idk how lmao
					g_pSquirrel<context>->AsyncCall("NSHandleLoadResult", handle, "");
					mutex.get().unlock();
					return;
				}

				std::stringstream stringStream;
				while (fileStr.peek() != EOF)
					stringStream << (char)fileStr.get();

				g_pSquirrel<context>->AsyncCall("NSHandleLoadResult", handle, stringStream.str());

				fileStr.close();
				mutex.get().unlock();
				// side-note: this causes a leak?
				// when a file is added to the map, it's never removed.
				// no idea how to fix this - because we have no way to check if there are other threads waiting to use this file(?)
				// tried to use m.try_lock(), but it's unreliable, it seems.
			}
			catch (std::exception ex)
			{
				spdlog::error("LOAD FAILED!");
				mutex.get().unlock();
				spdlog::error(ex.what());
			}
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
			try
			{
				m.get().lock();

				fs::remove(file);

				m.get().unlock();
				// side-note: this causes a leak?
				// when a file is added to the map, it's never removed.
				// no idea how to fix this - because we have no way to check if there are other threads waiting to use this file(?)
				// tried to use m.try_lock(), but it's unreliable, it seems.
			}
			catch (std::exception ex)
			{
				spdlog::error("DELETE FAILED!");
				m.get().unlock();
				spdlog::error(ex.what());
			}
		});

	deleteThread.detach();
}

bool ContainsInvalidChars(std::string str)
{
	// we don't allow null characters either, even if they're ASCII characters because idk if people can
	// use it to circumvent the file extension suffix.
	return std::any_of(str.begin(), str.end(), [](char c) { return c == '\0'; });
}
bool CheckFileName(fs::path str, fs::path dir)
{
	auto const normRoot = fs::weakly_canonical(dir);
	auto const normChild = fs::weakly_canonical(str);

	auto itr = std::search(normChild.begin(), normChild.end(), normRoot.begin(), normRoot.end());
	// we return if the file is NOT safe (NOT inside the directory)
	return itr != normChild.begin();
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

	fs::path dir = savePath / fs::path(mod->m_ModDirectory).filename();
	std::string fileName = g_pSquirrel<context>->getstring(sqvm, 1);
	if (CheckFileName(dir / fileName, dir))
	{
		g_pSquirrel<context>->raiseerror(
			sqvm,
			fmt::format("File name invalid ({})! Make sure it has no '\\', '/' or non-ASCII charcters!", fileName, mod->Name).c_str());
		return SQRESULT_ERROR;
	}

	std::string content = g_pSquirrel<context>->getstring(sqvm, 2);
	if (ContainsInvalidChars(content))
	{
		g_pSquirrel<context>->raiseerror(
			sqvm, fmt::format("File contents may not contain non-ASCII characters! Make sure your strings are valid!", mod->Name).c_str());
		return SQRESULT_ERROR;
	}

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

	fs::path dir = savePath / fs::path(mod->m_ModDirectory).filename();
	std::string fileName = g_pSquirrel<context>->getstring(sqvm, 1);
	if (CheckFileName(dir / fileName, dir))
	{
		g_pSquirrel<context>->raiseerror(
			sqvm,
			fmt::format("File name invalid ({})! Make sure it has no '\\', '/' or non-ASCII charcters!", fileName, mod->Name).c_str());
		return SQRESULT_ERROR;
	}

	std::string content = EncodeJSON<context>(sqvm);
	if (ContainsInvalidChars(content))
	{
		g_pSquirrel<context>->raiseerror(
			sqvm, fmt::format("File contents may not contain non-ASCII characters! Make sure your strings are valid!", mod->Name).c_str());
		return SQRESULT_ERROR;
	}

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

	fs::path dir = savePath / fs::path(mod->m_ModDirectory).filename();
	std::string fileName = g_pSquirrel<context>->getstring(sqvm, 1);
	if (CheckFileName(dir / fileName, dir))
	{
		g_pSquirrel<context>->raiseerror(
			sqvm,
			fmt::format("File name invalid ({})! Make sure it has no '\\', '/' or non-ASCII charcters!", fileName, mod->Name).c_str());
		return SQRESULT_ERROR;
	}

	g_pSquirrel<context>->pushinteger(sqvm, g_saveFileManager->LoadFileAsync<context>(dir / fileName));

	return SQRESULT_NOTNULL;
}

// bool NSDoesFileExist(string file)
ADD_SQFUNC("bool", NSDoesFileExist, "string file", "", ScriptContext::SERVER | ScriptContext::CLIENT | ScriptContext::UI)
{
	Mod* mod = g_pSquirrel<context>->getcallingmod(sqvm);

	fs::path dir = savePath / fs::path(mod->m_ModDirectory).filename();
	std::string fileName = g_pSquirrel<context>->getstring(sqvm, 1);
	if (CheckFileName(dir / fileName, dir))
	{
		g_pSquirrel<context>->raiseerror(
			sqvm,
			fmt::format("File name invalid ({})! Make sure it has no '\\', '/' or non-ASCII charcters!", fileName, mod->Name).c_str());
		return SQRESULT_ERROR;
	}

	g_pSquirrel<context>->pushbool(sqvm, fs::exists(dir / (fileName)));
	return SQRESULT_NOTNULL;
}

// int NSGetFileSize(string file)
ADD_SQFUNC("int", NSGetFileSize, "string file", "", ScriptContext::SERVER | ScriptContext::CLIENT | ScriptContext::UI)
{
	Mod* mod = g_pSquirrel<context>->getcallingmod(sqvm);

	fs::path dir = savePath / fs::path(mod->m_ModDirectory).filename();
	std::string fileName = g_pSquirrel<context>->getstring(sqvm, 1);
	if (CheckFileName(dir / fileName, dir))
	{
		g_pSquirrel<context>->raiseerror(
			sqvm,
			fmt::format("File name invalid ({})! Make sure it has no '\\', '/' or non-ASCII charcters!", fileName, mod->Name).c_str());
		return SQRESULT_ERROR;
	}
	try
	{
		// throws if file does not exist
		// we don't want stuff such as "file does not exist, file is unavailable" to be lethal, so we just try/catch fs errors
		g_pSquirrel<context>->pushinteger(sqvm, (int)(fs::file_size(dir / fileName) / 1024));
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

	fs::path dir = savePath / fs::path(mod->m_ModDirectory).filename();
	std::string fileName = g_pSquirrel<context>->getstring(sqvm, 1);
	if (CheckFileName(dir / fileName, dir))
	{
		g_pSquirrel<context>->raiseerror(
			sqvm,
			fmt::format("File name invalid ({})! Make sure it has no '\\', '/' or non-ASCII charcters!", fileName, mod->Name).c_str());
		return SQRESULT_ERROR;
	}

	g_saveFileManager->DeleteFileAsync<context>(dir / fileName);
	return SQRESULT_NOTNULL;
}

ADD_SQFUNC("array<string>", NSGetAllFiles, "string path = \"\"", "", ScriptContext::CLIENT | ScriptContext::UI | ScriptContext::SERVER)
{
	Mod* mod = g_pSquirrel<context>->getcallingmod(sqvm);
	fs::path dir = savePath / fs::path(mod->m_ModDirectory).filename();
	std::string pathStr = g_pSquirrel<context>->getstring(sqvm, 1);
	fs::path path = dir;
	if (pathStr != "")
		path = dir / pathStr;
	if (CheckFileName(path, dir))
	{
		g_pSquirrel<context>->raiseerror(
			sqvm, fmt::format("File name invalid ({})! Make sure it has no '\\', '/' or non-ASCII charcters!", pathStr, mod->Name).c_str());
		return SQRESULT_ERROR;
	}
	for (const auto& entry : fs::directory_iterator(path))
	{
		g_pSquirrel<context>->pushstring(sqvm, entry.path().filename().string().c_str());
		g_pSquirrel<context>->arrayappend(sqvm, -2);
	}
	return SQRESULT_NOTNULL;
}

ADD_SQFUNC("bool", NSIsFolder, "string path", "", ScriptContext::CLIENT | ScriptContext::UI | ScriptContext::SERVER)
{
	Mod* mod = g_pSquirrel<context>->getcallingmod(sqvm);
	fs::path dir = savePath / fs::path(mod->m_ModDirectory).filename();
	std::string pathStr = g_pSquirrel<context>->getstring(sqvm, 1);
	fs::path path = dir;
	if (pathStr != "")
		path = dir / pathStr;
	if (CheckFileName(path, dir))
	{
		g_pSquirrel<context>->raiseerror(
			sqvm, fmt::format("File name invalid ({})! Make sure it has no '\\', '/' or non-ASCII charcters!", pathStr, mod->Name).c_str());
		return SQRESULT_ERROR;
	}
	g_pSquirrel<context>->pushbool(sqvm, fs::is_directory(path));
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
	int parm = Tier0::CommandLine()->FindParm("-disablehttprequests");
	if (parm)
		MAX_FOLDER_SIZE = std::stoi(Tier0::CommandLine()->GetParm(parm));
}
