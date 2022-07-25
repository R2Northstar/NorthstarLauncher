//=============================================================================//
//
// Purpose:
//
// $NoKeywords: $
//
//=============================================================================//
// server.cpp: implementation of the CServer class.
//
/////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "server.h"
#include "client.h"

//---------------------------------------------------------------------------------
// Purpose: Gets the number of human players on the server
// Output : int
//---------------------------------------------------------------------------------
int CServer::GetNumHumanPlayers(void) const
{
	int nHumans = 0;
	for (int i = 0; i < 32; i++) // !FIXME: Use g_ServerGlobalVariables->m_nMaxClients!
	{
		CClient* pClient = g_pClient->GetClient(i);
		if (!pClient)
			continue;

		if (pClient->IsHumanPlayer())
			nHumans++;
	}

	return nHumans;
}

//---------------------------------------------------------------------------------
// Purpose: Gets the number of fake clients on the server
// Output : int
//---------------------------------------------------------------------------------
int CServer::GetNumFakeClients(void) const
{
	int nBots = 0;
	for (int i = 0; i < 32; i++) // !FIXME: Use g_ServerGlobalVariables->m_nMaxClients!
	{
		CClient* pClient = g_pClient->GetClient(i);
		if (!pClient)
			continue;

		if (pClient->IsConnected() && pClient->IsFakeClient())
			nBots++;
	}

	return nBots;
}

///////////////////////////////////////////////////////////////////////////////
CServer* g_pServer = nullptr; // Link this to the global server array below.

void InitialiseBaseServer(HMODULE baseAddress) {

}