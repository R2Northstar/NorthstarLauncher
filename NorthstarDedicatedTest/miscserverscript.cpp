#include "pch.h"
#include "miscserverscript.h"
#include "squirrel.h"
#include "masterserver.h"
#include "serverauthentication.h"
#include "gameutils.h"
#include <iostream>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

// annoying helper function because i can't figure out getting players or entities from sqvm rn
// wish i didn't have to do it like this, but here we are
void* GetPlayerByIndex(int playerIndex)
{
	const int PLAYER_ARRAY_OFFSET = 0x12A53F90;
	const int PLAYER_SIZE = 0x2D728;

	void* playerArrayBase = (char*)GetModuleHandleA("engine.dll") + PLAYER_ARRAY_OFFSET;
	void* player = (char*)playerArrayBase + (playerIndex * PLAYER_SIZE);

	return player;
}

// void function NSEarlyWritePlayerIndexPersistenceForLeave( int playerIndex )
SQRESULT SQ_EarlyWritePlayerIndexPersistenceForLeave(void* sqvm)
{
	int playerIndex = ServerSq_getinteger(sqvm, 1);
	void* player = GetPlayerByIndex(playerIndex);

	if (!g_ServerAuthenticationManager->m_additionalPlayerData.count(player))
	{
		ServerSq_pusherror(sqvm, fmt::format("Invalid playerindex {}", playerIndex).c_str());
		return SQRESULT_ERROR;
	}

	g_ServerAuthenticationManager->m_additionalPlayerData[player].needPersistenceWriteOnLeave = false;
	g_ServerAuthenticationManager->WritePersistentData(player);
	return SQRESULT_NULL;
}

// bool function NSIsWritingPlayerPersistence()
SQRESULT SQ_IsWritingPlayerPersistence(void* sqvm)
{
	ServerSq_pushbool(sqvm, g_MasterServerManager->m_savingPersistentData);
	return SQRESULT_NOTNULL;
}

// bool function NSIsPlayerIndexLocalPlayer( int playerIndex )
SQRESULT SQ_IsPlayerIndexLocalPlayer(void* sqvm)
{
	int playerIndex = ServerSq_getinteger(sqvm, 1);
	void* player = GetPlayerByIndex(playerIndex);
	if (!g_ServerAuthenticationManager->m_additionalPlayerData.count(player))
	{
		ServerSq_pusherror(sqvm, fmt::format("Invalid playerindex {}", playerIndex).c_str());
		return SQRESULT_ERROR;
	}

	ServerSq_pushbool(sqvm, !strcmp(g_LocalPlayerUserID, (char*)player + 0xF500));
	return SQRESULT_NOTNULL;
}

// For all of those about to file a security issue because people would be able to override system files
// or bloat up the system immensely:
//
// THIS IS ALREADY A PROBLEM, EVEN IF THESE FUNCTIONS DID NOT EXIST.
// Why? See DevTextBufferWrite, DevTextBufferClear, and DevTextBufferDumpToFile.
// as long as someone keeps sv_cheats to 0 - and doesn't allow the user to write files on their own - THIS SHOULD BE FINE.
SQRESULT SQ_WriteFile(void* sqvm)
{
	// mod name for making selected directory :D
	const SQChar* modName = ServerSq_getstring(sqvm, 1);
	const SQChar* path = ServerSq_getstring(sqvm, 2);
	const SQChar* fileName = ServerSq_getstring(sqvm, 3);
	const SQChar* content = ServerSq_getstring(sqvm, 4);
	try
	{
		std::ofstream myfile;
		std::string finalPath;
		finalPath += "R2Northstar/Persistence/";
		finalPath += modName;
		if (path[0] != '/')
			finalPath += '/';
		finalPath += path;
		std::filesystem::create_directories(finalPath);

		if (finalPath[finalPath.length() - 1] != '/' && fileName[0] != '/')
			finalPath += '/';

		finalPath += fileName;
		if (finalPath.find("..") != std::string::npos)
		{
			ServerSq_pusherror(sqvm, "Attempted to access higher directory!!!!");
			return SQRESULT_ERROR;
		}
		spdlog::info("PATH: {} | CONTENT: {}", finalPath, content);
		myfile.open(finalPath.c_str());
		if (!myfile)
		{
			ServerSq_pusherror(sqvm, fmt::format("File could not be opened!").c_str());
			return SQRESULT_ERROR;
		}
		myfile << content;
		myfile.close();
	}
	catch (...)
	{
		ServerSq_pusherror(sqvm, fmt::format("Error whilst writing to file!").c_str());
		return SQRESULT_ERROR;
	}
	return SQRESULT_NULL;
}

SQRESULT SQ_ReadFile(void* sqvm)
{
	std::ifstream myfile;

	const SQChar* modName = ServerSq_getstring(sqvm, 1);
	const SQChar* path = ServerSq_getstring(sqvm, 2);
	const SQChar* fileName = ServerSq_getstring(sqvm, 3);

	try
	{
		std::string result;

		std::string finalPath;
		finalPath += "R2Northstar/Persistence/";
		finalPath += modName;
		if (path[0] != '/') finalPath += '/';
		finalPath += path;
		if (finalPath[finalPath.length() - 1] != '/' && fileName[0] != '/')
			finalPath += '/';
		finalPath += fileName;
		if (finalPath.find("..") != std::string::npos)
		{
			ServerSq_pusherror(sqvm, "Attempted to access higher directory!!!!");
			return SQRESULT_ERROR;
		}

		myfile.open(finalPath.c_str());

		if (!myfile)
		{
			ServerSq_pusherror(sqvm, "File could not be opened!");
			return SQRESULT_ERROR;
		}

		myfile.seekg(0, std::ios::end);
		result.reserve(myfile.tellg());
		myfile.seekg(0, std::ios::beg);

		result.assign((std::istreambuf_iterator<char>(myfile)), std::istreambuf_iterator<char>());

		ServerSq_pushstring(sqvm, result.c_str(), -1);
	}
	catch (...)
	{
		ServerSq_pusherror(sqvm, fmt::format("Error whilst reading from file {} (make sure it exists!)", path).c_str());
		return SQRESULT_ERROR;
	}

	return SQRESULT_NOTNULL;
}

SQRESULT SQ_GetAllFilesInFolder(void* sqvm)
{

	const SQChar* modName = ServerSq_getstring(sqvm, 1);
	const SQChar* path = ServerSq_getstring(sqvm, 2);

	ServerSq_newarray(sqvm, 0);

	try
	{
		std::string result;

		std::string finalPath;
		finalPath += "R2Northstar/Persistence/";
		finalPath += modName;
		finalPath += "/";
		finalPath += path;
		if (finalPath.find("..") != std::string::npos)
		{
			ServerSq_pusherror(sqvm, "Attempted to access higher directory!!!!");
			return SQRESULT_ERROR;
		}

		using iterator = fs::directory_iterator;

		const iterator end;
		for (iterator iter{finalPath}; iter != end; ++iter)
		{
			if (iter->is_directory())
				continue;
			ServerSq_pushstring(sqvm, iter->path().filename().string().c_str(), -1);
			ServerSq_arrayappend(sqvm, -2);
		}
	}
	catch (...)
	{
		ServerSq_pusherror(sqvm, fmt::format("Error whilst reading from folder {} (make sure it exists!)", path).c_str());
		return SQRESULT_ERROR;
	}
	return SQRESULT_NOTNULL;
}

SQRESULT SQ_GetAllFoldersInFolder(void* sqvm)
{

	const SQChar* modName = ServerSq_getstring(sqvm, 1);
	const SQChar* path = ServerSq_getstring(sqvm, 2);

	ServerSq_newarray(sqvm, 0);

	try
	{
		std::string result;

		std::string finalPath;
		finalPath += "R2Northstar/Persistence/";
		finalPath += modName;
		finalPath += "/";
		finalPath += path;
		if (finalPath.find("..") != std::string::npos)
		{
			ServerSq_pusherror(sqvm, "Attempted to access higher directory!!!!");
			return SQRESULT_ERROR;
		}

		if (ServerSq_getbool(sqvm, 3))
		{
			using iterator = fs::recursive_directory_iterator;

			const iterator end;
			for (iterator iter{finalPath}; iter != end; ++iter)
			{
				if (!iter->is_directory())
					continue;
				ServerSq_pushstring(sqvm, iter->path().string().c_str(), -1);
				ServerSq_arrayappend(sqvm, -2);
			}
		}
		else
		{
			using iterator = fs::directory_iterator;

			const iterator end;
			for (iterator iter{finalPath}; iter != end; ++iter)
			{
				if (!iter->is_directory())
					continue;
				ServerSq_pushstring(sqvm, iter->path().string().c_str(), -1);
				ServerSq_arrayappend(sqvm, -2);
			}
		}
	}
	catch (...)
	{
		ServerSq_pusherror(sqvm, fmt::format("Error whilst reading from folder {} (make sure it exists!)", path).c_str());
		return SQRESULT_ERROR;
	}
	return SQRESULT_NOTNULL;
}
void InitialiseMiscServerScriptCommand(HMODULE baseAddress)
{
	g_ServerSquirrelManager->AddFuncRegistration(
		"void", "NSEarlyWritePlayerIndexPersistenceForLeave", "int playerIndex", "", SQ_EarlyWritePlayerIndexPersistenceForLeave);
	g_ServerSquirrelManager->AddFuncRegistration("bool", "NSIsWritingPlayerPersistence", "", "", SQ_IsWritingPlayerPersistence);
	g_ServerSquirrelManager->AddFuncRegistration("bool", "NSIsPlayerIndexLocalPlayer", "int playerIndex", "", SQ_IsPlayerIndexLocalPlayer);
	g_ServerSquirrelManager->AddFuncRegistration(
		"void", "WriteFile", "string modName, string path, string fileName, string content", "", SQ_WriteFile);
	g_ServerSquirrelManager->AddFuncRegistration("string", "ReadFile", "string modName, string path, string fileName", "", SQ_ReadFile);
	g_ServerSquirrelManager->AddFuncRegistration(
		"array<string>", "GetAllFilesInFolder", "string modName, string path", "", SQ_GetAllFilesInFolder);
	g_ServerSquirrelManager->AddFuncRegistration(
		"array<string>", "GetAllFoldersInFolder", "string modName, string path, bool recursive = false", "", SQ_GetAllFoldersInFolder);
}
