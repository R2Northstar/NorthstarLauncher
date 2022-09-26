#pragma once
#include "pch.h"
#include "namedpipeclient.h"
#include "squirrel.h" // Squirrel

// Named Pipe Stuff
#include <windows.h>
#include <conio.h>
#include <tchar.h>
#include <string>
#include <atlstr.h>

using namespace std;

#define GENERAL_PIPE_NAME TEXT("\\\\.\\pipe\\GameDataPipe")
#define BUFF_SIZE 512

HANDLE hPipe;
bool isConnected = false;

ConVar* Cvar_ns_enable_named_pipe;

HANDLE GetNewPipeInstance()
{
	HANDLE generalPipe = CreateFile(GENERAL_PIPE_NAME, GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	bool isGeneralConnected = generalPipe != INVALID_HANDLE_VALUE;
	HANDLE thisMatchPipe = INVALID_HANDLE_VALUE;
	if (isGeneralConnected)
	{
		TCHAR chBuf[BUFF_SIZE]; // TODO: Length of general pipe ids + terminating zero
		DWORD cbRead;
		if (ReadFile(
				generalPipe, // pipe handle
				chBuf, // buffer to receive reply
				BUFF_SIZE * sizeof(TCHAR), // size of buffer
				&cbRead, // number of bytes read
				NULL)) // not overlapped
		{
			do
			{
				spdlog::info("New Pipe connection");
				thisMatchPipe = CreateFile(chBuf, GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
			} while (thisMatchPipe == INVALID_HANDLE_VALUE);
		}
	}
	return thisMatchPipe;
}

void SendMessageToPipe(string message)
{
	bool success = false;
	DWORD read;
	do
	{
		success = WriteFile(hPipe, message.c_str(), message.length(), &read, nullptr);
	} while (!success);
}

SQRESULT SQ_SendToNamedPipe(void* sqvm)
{
	try
	{
		if (Cvar_ns_enable_named_pipe->GetInt() && isConnected)
		{
			SendMessageToPipe(ServerSq_getstring(sqvm, 1));
		}
	}
	catch (exception _e)
	{
		spdlog::error("Internal error while sending message through named pipe");
		spdlog::error(_e.what());
	}

	return SQRESULT_NULL;
}

SQRESULT SQ_OpenNamedPipe(void* sqvm)
{
	try
	{
		if (Cvar_ns_enable_named_pipe->GetInt())
		{
			if (!isConnected || hPipe == INVALID_HANDLE_VALUE)
			{
				isConnected = false;
				hPipe = GetNewPipeInstance();
			}
			else
			{
				SendMessageToPipe(ServerSq_getstring(sqvm, 2));
			}

			isConnected = hPipe != INVALID_HANDLE_VALUE;

			if (isConnected)
			{
				SendMessageToPipe(ServerSq_getstring(sqvm, 1));
			}
			else
			{
				spdlog::error("INVALID_HANDLE_VALUE while opening named pipe");
				spdlog::error(GetLastError());
			}
		}
	}
	catch (exception _e)
	{
		spdlog::error("Internal error while opening named pipe");
		spdlog::error(_e.what());
	}

	return SQRESULT_NULL;
}

SQRESULT SQ_ClosePipe(void* sqvm)
{
	try
	{
		if (Cvar_ns_enable_named_pipe->GetInt() && isConnected)
		{
			SendMessageToPipe(ServerSq_getstring(sqvm, 1));

			CloseHandle(hPipe);
			isConnected = false;
		}
	}
	catch (exception _e)
	{
		spdlog::error("Internal error while closing named pipe");
		spdlog::error(_e.what());
	}

	return SQRESULT_NULL;
}

void InitialiseNamedPipeClient(HMODULE baseAddress)
{
	Cvar_ns_enable_named_pipe =
		new ConVar("ns_enable_named_pipe", "0", FCVAR_GAMEDLL, "Whether to start up a named pipe server on request from squirrel");
	g_ServerSquirrelManager->AddFuncRegistration("void", "NSSendToNamedPipe", "string textToSend", "", SQ_SendToNamedPipe);
	g_ServerSquirrelManager->AddFuncRegistration(
		"void", "NSOpenNamedPipe", "string openingMessage, string closingMessageIfOpen", "", SQ_OpenNamedPipe);
	g_ServerSquirrelManager->AddFuncRegistration("void", "NSCloseNamedPipe", "string closingMessage", "", SQ_ClosePipe);
}
