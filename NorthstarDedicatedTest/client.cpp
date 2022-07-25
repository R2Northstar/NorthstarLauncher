//===============================================================================//
//
// Purpose:
//
// $NoKeywords: $
//
//===============================================================================//
// client.cpp: implementation of the CClient class.
//
///////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "client.h"

//---------------------------------------------------------------------------------
// Purpose: gets the client from buffer by index
//---------------------------------------------------------------------------------
CClient* CClient::GetClient(int nIndex) const
{
	return reinterpret_cast<CClient*>((reinterpret_cast<uintptr_t>(g_pClient) + (nIndex * sizeof(CClient))));
}

//---------------------------------------------------------------------------------
// Purpose: gets the handle of this client
//---------------------------------------------------------------------------------
uint16_t CClient::GetHandle(void) const
{
	return m_nHandle;
}

//---------------------------------------------------------------------------------
// Purpose: gets the userID of this client
//---------------------------------------------------------------------------------
uint32_t CClient::GetUserID(void) const
{
	return m_nUserID;
}

//---------------------------------------------------------------------------------
// Purpose: gets the originID of this client
//---------------------------------------------------------------------------------
uint64_t CClient::GetOriginID(void) const
{
	return m_nOriginID;
}

//---------------------------------------------------------------------------------
// Purpose: gets the signon state of this client
//---------------------------------------------------------------------------------
SIGNONSTATE CClient::GetSignonState(void) const
{
	return m_nSignonState;
}

//---------------------------------------------------------------------------------
// Purpose: gets the persistence state of this client
//---------------------------------------------------------------------------------
PERSISTENCE CClient::GetPersistenceState(void) const
{
	return m_nPersistenceState;
}

//---------------------------------------------------------------------------------
// Purpose: gets the net channel of this client
//---------------------------------------------------------------------------------
CNetChan* CClient::GetNetChan(void) const
{
	return m_NetChannel;
}

//---------------------------------------------------------------------------------
// Purpose: gets the name of this client (managed by server)
//---------------------------------------------------------------------------------
const char* CClient::GetServerName(void) const
{
	return m_szServerName;
}

//---------------------------------------------------------------------------------
// Purpose: gets the name of this client (obtained from connectionless packet)
//---------------------------------------------------------------------------------
const char* CClient::GetClientName(void) const
{
	return m_szClientName;
}

//---------------------------------------------------------------------------------
// Purpose: sets the handle of this client
//---------------------------------------------------------------------------------
void CClient::SetHandle(uint16_t nHandle)
{
	m_nHandle = nHandle;
}

//---------------------------------------------------------------------------------
// Purpose: sets the userID of this client
//---------------------------------------------------------------------------------
void CClient::SetUserID(uint32_t nUserID)
{
	m_nUserID = nUserID;
}

//---------------------------------------------------------------------------------
// Purpose: sets the originID of this client
//---------------------------------------------------------------------------------
void CClient::SetOriginID(uint64_t nOriginID)
{
	m_nOriginID = nOriginID;
}

//---------------------------------------------------------------------------------
// Purpose: sets the signon state of this client
//---------------------------------------------------------------------------------
void CClient::SetSignonState(SIGNONSTATE nSignonState)
{
	m_nSignonState = nSignonState;
}

//---------------------------------------------------------------------------------
// Purpose: sets the persistence state of this client
//---------------------------------------------------------------------------------
void CClient::SetPersistenceState(PERSISTENCE nPersistenceState)
{
	m_nPersistenceState = nPersistenceState;
}

//---------------------------------------------------------------------------------
// Purpose: sets the net channel of this client
// !TODO  : Remove this and rebuild INetChannel
//---------------------------------------------------------------------------------
void CClient::SetNetChan(CNetChan* pNetChan)
{
	m_NetChannel = pNetChan;
}

//---------------------------------------------------------------------------------
// Purpose: checks if client is connected to server
// Output : true if connected, false otherwise
//---------------------------------------------------------------------------------
bool CClient::IsConnected(void) const
{
	return m_nSignonState >= SIGNONSTATE::SIGNONSTATE_CONNECTED;
}

//---------------------------------------------------------------------------------
// Purpose: checks if client is spawned to server
// Output : true if connected, false otherwise
//---------------------------------------------------------------------------------
bool CClient::IsSpawned(void) const
{
	return m_nSignonState >= SIGNONSTATE::SIGNONSTATE_NEW;
}

//---------------------------------------------------------------------------------
// Purpose: checks if client is active to server
// Output : true if connected, false otherwise
//---------------------------------------------------------------------------------
bool CClient::IsActive(void) const
{
	return m_nSignonState == SIGNONSTATE::SIGNONSTATE_FULL;
}
//---------------------------------------------------------------------------------
// Purpose: checks if client's persistence data is ready
// Output : true if ready, false otherwise
//---------------------------------------------------------------------------------
bool CClient::IsPersistenceReady(void) const
{
	return m_nPersistenceState == PERSISTENCE::PERSISTENCE_READY;
}

//---------------------------------------------------------------------------------
// Purpose: checks if client is a fake client
// Output : true if connected, false otherwise
//---------------------------------------------------------------------------------
bool CClient::IsFakeClient(void) const
{
	return m_bFakePlayer;
}

//---------------------------------------------------------------------------------
// Purpose: checks if this client is an actual human player
// Output : true if human, false otherwise
//---------------------------------------------------------------------------------
bool CClient::IsHumanPlayer(void) const
{
	if (!IsConnected())
		return false;

	if (IsFakeClient())
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
CClient* g_pClient = nullptr;

void InitialiseBaseClient(HMODULE baseAddress)
{
	g_pClient = (CClient*)GetModuleHandleA("engine.dll") + 0x12A53F90;
}