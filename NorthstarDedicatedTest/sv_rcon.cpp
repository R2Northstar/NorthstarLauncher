//===========================================================================//
//
// Purpose: Implementation of the rcon server.
//
//===========================================================================//

#include "pch.h"
#include "concommand.h"
#include "cvar.h"
#include "convar.h"
#include "net.h"
#include "NetAdr2.h"
#include "socketcreator.h"
#include "rcon_shared.h"
#include "sv_rcon.h"
#include "sv_rcon.pb.h"
#include "cl_rcon.pb.h"
#include "sha256.h"
#include "gameutils.h"
#include "igameserverdata.h"

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CRConServer::CRConServer(void) : m_bInitialized(false), m_nConnIndex(0)
{
	m_pAdr2 = new CNetAdr2();
	m_pSocket = new CSocketCreator();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CRConServer::~CRConServer(void)
{
	delete m_pAdr2;
	delete m_pSocket;
}
//-----------------------------------------------------------------------------
// Purpose: NETCON systems init
//-----------------------------------------------------------------------------
void CRConServer::Init(void)
{
	if (!m_bInitialized)
	{
		if (!this->SetPassword(CVar_rcon_password->GetString()))
		{
			return;
		}
	}

	static ConVar* hostport = g_pCVar->FindVar("hostport");

	m_pAdr2->SetIPAndPort(CVar_rcon_address->GetString(), hostport->GetString());
	m_pSocket->CreateListenSocket(*m_pAdr2, false);

	spdlog::info("Remote server access initialized");
	m_bInitialized = true;
}

//-----------------------------------------------------------------------------
// Purpose: NETCON systems shutdown
//-----------------------------------------------------------------------------
void CRConServer::Shutdown(void)
{
	if (m_pSocket->IsListening())
	{
		m_pSocket->CloseListenSocket();
	}
	m_bInitialized = false;
}

//-----------------------------------------------------------------------------
// Purpose: run tasks for the RCON server
//-----------------------------------------------------------------------------
void CRConServer::Think(void)
{
	int nCount = m_pSocket->GetAcceptedSocketCount();

	// Close redundant sockets if there are too many except for whitelisted and authenticated.
	if (nCount >= CVar_sv_rcon_maxsockets->GetInt())
	{
		for (m_nConnIndex = nCount - 1; m_nConnIndex >= 0; m_nConnIndex--)
		{
			CNetAdr2 netAdr2 = m_pSocket->GetAcceptedSocketAddress(m_nConnIndex);
			if (std::strcmp(netAdr2.GetIP(true).c_str(), CVar_sv_rcon_whitelist_address->GetString()) != 0)
			{
				CConnectedNetConsoleData* pData = m_pSocket->GetAcceptedSocketData(m_nConnIndex);
				if (!pData->m_bAuthorized)
				{
					this->CloseConnection();
				}
			}
		}
	}

	// Create a new listen socket if authenticated connection is closed.
	if (nCount == 0)
	{
		if (!m_pSocket->IsListening())
		{
			m_pSocket->CreateListenSocket(*m_pAdr2, false);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: changes the password
// Input  : *pszPassword -
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CRConServer::SetPassword(const char* pszPassword)
{
	m_bInitialized = false;
	m_pSocket->CloseAllAcceptedSockets();

	if (std::strlen(pszPassword) < 8)
	{
		if (std::strlen(pszPassword) > 0)
		{
			spdlog::warn("Remote server access requires a password of at least 8 characters");
		}
		this->Shutdown();
		return false;
	}
	m_svPasswordHash = sha256(pszPassword);
	spdlog::info("Password hash ('{:s}')", m_svPasswordHash);

	m_bInitialized = true;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: server RCON main loop (run this every frame)
//-----------------------------------------------------------------------------
void CRConServer::RunFrame(void)
{
	if (m_bInitialized)
	{
		m_pSocket->RunFrame();
		this->Think();
		this->Recv();
	}
}

//-----------------------------------------------------------------------------
// Purpose: send message to all connected sockets
// Input  : *svMessage -
//-----------------------------------------------------------------------------
void CRConServer::Send(const std::string& svMessage) const
{
	if (int nCount = m_pSocket->GetAcceptedSocketCount())
	{
		std::ostringstream ssSendBuf;

		ssSendBuf << static_cast<uint8_t>(static_cast<int>(svMessage.size()) >> 24);
		ssSendBuf << static_cast<uint8_t>(static_cast<int>(svMessage.size()) >> 16);
		ssSendBuf << static_cast<uint8_t>(static_cast<int>(svMessage.size()) >> 8);
		ssSendBuf << static_cast<uint8_t>(static_cast<int>(svMessage.size()));
		ssSendBuf << svMessage;

		for (int i = nCount - 1; i >= 0; i--)
		{
			CConnectedNetConsoleData* pData = m_pSocket->GetAcceptedSocketData(i);
			if (pData->m_bAuthorized)
			{
				::send(pData->m_hSocket, ssSendBuf.str().data(), static_cast<int>(ssSendBuf.str().size()), MSG_NOSIGNAL);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: send message to specific connected socket
// Input  : hSocket -
//			*svMessage -
//-----------------------------------------------------------------------------
void CRConServer::Send(SocketHandle_t hSocket, const std::string& svMessage) const
{
	std::ostringstream ssSendBuf;

	ssSendBuf << static_cast<uint8_t>(static_cast<int>(svMessage.size()) >> 24);
	ssSendBuf << static_cast<uint8_t>(static_cast<int>(svMessage.size()) >> 16);
	ssSendBuf << static_cast<uint8_t>(static_cast<int>(svMessage.size()) >> 8);
	ssSendBuf << static_cast<uint8_t>(static_cast<int>(svMessage.size()));
	ssSendBuf << svMessage;

	::send(hSocket, ssSendBuf.str().data(), static_cast<int>(ssSendBuf.str().size()), MSG_NOSIGNAL);
}

//-----------------------------------------------------------------------------
// Purpose: receive message
//-----------------------------------------------------------------------------
void CRConServer::Recv(void)
{
	int nCount = m_pSocket->GetAcceptedSocketCount();
	static char szRecvBuf[1024];

	for (m_nConnIndex = nCount - 1; m_nConnIndex >= 0; m_nConnIndex--)
	{
		CConnectedNetConsoleData* pData = m_pSocket->GetAcceptedSocketData(m_nConnIndex);
		{ //////////////////////////////////////////////
			if (this->CheckForBan(pData))
			{
				this->Send(pData->m_hSocket, this->Serialize(s_pszBannedMessage, "", sv_rcon::response_t::SERVERDATA_RESPONSE_AUTH));
				this->CloseConnection();
				continue;
			}

			int nPendingLen = ::recv(pData->m_hSocket, szRecvBuf, sizeof(char), MSG_PEEK);
			if (nPendingLen == SOCKET_ERROR && m_pSocket->IsSocketBlocking())
			{
				continue;
			}
			if (nPendingLen <= 0) // EOF or error.
			{
				this->CloseConnection();
				continue;
			}
		} //////////////////////////////////////////////

		u_long nReadLen; // Find out how much we have to read.
		::ioctlsocket(pData->m_hSocket, FIONREAD, &nReadLen);

		while (nReadLen > 0)
		{
			int nRecvLen = ::recv(pData->m_hSocket, szRecvBuf, MIN(sizeof(szRecvBuf), nReadLen), MSG_NOSIGNAL);
			if (nRecvLen == 0) // Socket was closed.
			{
				this->CloseConnection();
				break;
			}
			if (nRecvLen < 0 && !m_pSocket->IsSocketBlocking())
			{
				spdlog::error("RCON Cmd: recv error ({:s})", NET_ErrorString(WSAGetLastError()));
				break;
			}

			nReadLen -= nRecvLen; // Process what we've got.
			this->ProcessBuffer(szRecvBuf, nRecvLen, pData);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: serializes input
// Input  : *svRspBuf -
//			*svRspVal -
//			response_t -
// Output : serialized results as string
//-----------------------------------------------------------------------------
std::string CRConServer::Serialize(const std::string& svRspBuf, const std::string& svRspVal, sv_rcon::response_t response_t) const
{
	sv_rcon::response sv_response;

	sv_response.set_responseid(-1); // TODO
	sv_response.set_responsetype(response_t);

	switch (response_t)
	{
	case sv_rcon::response_t::SERVERDATA_RESPONSE_AUTH:
	{
		sv_response.set_responsebuf(svRspBuf);
		break;
	}
	case sv_rcon::response_t::SERVERDATA_RESPONSE_CONSOLE_LOG:
	{
		sv_response.set_responsebuf(svRspBuf);
		sv_response.set_responseval("");
		break;
	}
	default:
	{
		break;
	}
	}
	return sv_response.SerializeAsString();
}

//-----------------------------------------------------------------------------
// Purpose: de-serializes input
// Input  : *svBuf -
// Output : de-serialized object
//-----------------------------------------------------------------------------
cl_rcon::request CRConServer::Deserialize(const std::string& svBuf) const
{
	cl_rcon::request cl_request;
	cl_request.ParseFromArray(svBuf.data(), static_cast<int>(svBuf.size()));

	return cl_request;
}

//-----------------------------------------------------------------------------
// Purpose: authenticate new connections
// Input  : *cl_request -
//			*pData -
//-----------------------------------------------------------------------------
void CRConServer::Authenticate(const cl_rcon::request& cl_request, CConnectedNetConsoleData* pData)
{
	if (pData->m_bAuthorized)
	{
		return;
	}
	else // Authorize.
	{
		if (this->Comparator(cl_request.requestbuf()))
		{
			pData->m_bAuthorized = true;
			m_pSocket->CloseListenSocket();

			this->CloseNonAuthConnection();
			this->Send(pData->m_hSocket, this->Serialize(s_pszAuthMessage, "", sv_rcon::response_t::SERVERDATA_RESPONSE_AUTH));
		}
		else // Bad password.
		{
			CNetAdr2 netAdr2 = m_pSocket->GetAcceptedSocketAddress(m_nConnIndex);
			if (CVar_sv_rcon_debug->GetBool())
			{
				spdlog::info("Bad RCON password attempt from '{:s}'", netAdr2.GetIPAndPort());
			}

			this->Send(pData->m_hSocket, this->Serialize(s_pszWrongPwMessage, "", sv_rcon::response_t::SERVERDATA_RESPONSE_AUTH));

			pData->m_bAuthorized = false;
			pData->m_bValidated = false;
			pData->m_nFailedAttempts++;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: sha256 hashed password comparison
// Input  : *svCompare -
// Output : true if matches, false otherwise
//-----------------------------------------------------------------------------
bool CRConServer::Comparator(std::string svPassword) const
{
	svPassword = sha256(svPassword);
	if (CVar_sv_rcon_debug->GetBool())
	{
		spdlog::info("+---------------------------------------------------------------------------+");
		spdlog::info("] Server: '{:s}'[", m_svPasswordHash);
		spdlog::info("] Client: '{:s}'[", svPassword);
		spdlog::info("+---------------------------------------------------------------------------+");
	}
	if (memcmp(svPassword.c_str(), m_svPasswordHash.c_str(), SHA256::DIGEST_SIZE) == 0)
	{
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: parses input response buffer using length-prefix framing
// Input  : *pRecvBuf -
//			nRecvLen -
//			*pData -
//-----------------------------------------------------------------------------
void CRConServer::ProcessBuffer(const char* pRecvBuf, int nRecvLen, CConnectedNetConsoleData* pData)
{
	while (nRecvLen > 0)
	{
		if (pData->m_nPayloadLen)
		{
			if (pData->m_nPayloadRead < pData->m_nPayloadLen)
			{
				pData->m_RecvBuffer[pData->m_nPayloadRead++] = *pRecvBuf;

				pRecvBuf++;
				nRecvLen--;
			}
			if (pData->m_nPayloadRead == pData->m_nPayloadLen)
			{
				this->ProcessMessage(
					this->Deserialize(std::string(reinterpret_cast<char*>(pData->m_RecvBuffer.data()), pData->m_nPayloadLen)));

				pData->m_nPayloadLen = 0;
				pData->m_nPayloadRead = 0;
			}
		}
		else if (pData->m_nPayloadRead < sizeof(int)) // Read size field.
		{
			pData->m_RecvBuffer[pData->m_nPayloadRead++] = *pRecvBuf;

			pRecvBuf++;
			nRecvLen--;
		}
		else // Build prefix.
		{
			pData->m_nPayloadLen = static_cast<int>(
				pData->m_RecvBuffer[0] << 24 | pData->m_RecvBuffer[1] << 16 | pData->m_RecvBuffer[2] << 8 | pData->m_RecvBuffer[3]);
			pData->m_nPayloadRead = 0;

			if (!pData->m_bAuthorized)
			{
				if (pData->m_nPayloadLen > MAX_NETCONSOLE_INPUT_LEN)
				{
					this->CloseConnection(); // Sending large messages while not authenticated.
					break;
				}
			}

			if (pData->m_nPayloadLen < 0)
			{
				spdlog::error("RCON Cmd: sync error ({:d})", pData->m_nPayloadLen);
				this->CloseConnection(); // Out of sync (irrecoverable).

				break;
			}
			else
			{
				pData->m_RecvBuffer.resize(pData->m_nPayloadLen);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: processes received message
// Input  : *cl_request -
//-----------------------------------------------------------------------------
void CRConServer::ProcessMessage(const cl_rcon::request& cl_request)
{
	CConnectedNetConsoleData* pData = m_pSocket->GetAcceptedSocketData(m_nConnIndex);

	if (!pData->m_bAuthorized && cl_request.requesttype() != cl_rcon::request_t::SERVERDATA_REQUEST_AUTH)
	{
		// Notify net console that authentication is required.
		this->Send(pData->m_hSocket, this->Serialize(s_pszNoAuthMessage, "", sv_rcon::response_t::SERVERDATA_RESPONSE_AUTH));

		pData->m_bValidated = false;
		pData->m_nIgnoredMessage++;
		return;
	}
	switch (cl_request.requesttype())
	{
	case cl_rcon::request_t::SERVERDATA_REQUEST_AUTH:
	{
		this->Authenticate(cl_request, pData);
		break;
	}
	case cl_rcon::request_t::SERVERDATA_REQUEST_EXECCOMMAND:
	{
		if (pData->m_bAuthorized) // Only execute if auth was succesfull.
		{
			this->Execute(cl_request, false);
		}
		break;
	}
	case cl_rcon::request_t::SERVERDATA_REQUEST_SETVALUE:
	{
		if (pData->m_bAuthorized)
		{
			this->Execute(cl_request, true);
		}
		break;
	}
	case cl_rcon::request_t::SERVERDATA_REQUEST_SEND_CONSOLE_LOG:
	{
		if (pData->m_bAuthorized)
		{
			CVar_sv_rcon_sendlogs->SetValue(1);
		}
		break;
	}
	default:
	{
		break;
	}
	}
}

//-----------------------------------------------------------------------------
// Purpose: execute commands issued from net console
// Input  : *cl_request -
//			bConVar -
//-----------------------------------------------------------------------------
void CRConServer::Execute(const cl_rcon::request& cl_request, bool bConVar) const
{
	if (bConVar)
	{
		ConVar* pConVar = g_pCVar->FindVar(cl_request.requestbuf().c_str());
		if (pConVar) // Set value without running the callback.
		{
			pConVar->SetValue(cl_request.requestval().c_str());
		}
	}
	else // Execute command with "<val>".
	{
		Cbuf_AddText(Cbuf_GetCurrentPlayer(), cl_request.requestbuf().c_str(), cmd_source_t::kCommandSrcCode);
		Cbuf_Execute();
	}
}

//-----------------------------------------------------------------------------
// Purpose: checks for amount of failed attempts and bans net console accordingly
// Input  : *pData -
//-----------------------------------------------------------------------------
bool CRConServer::CheckForBan(CConnectedNetConsoleData* pData)
{
	if (pData->m_bValidated)
	{
		return false;
	}

	pData->m_bValidated = true;
	CNetAdr2 netAdr2 = m_pSocket->GetAcceptedSocketAddress(m_nConnIndex);

	// Check if IP is in the ban vector.
	if (std::find(m_vBannedAddress.begin(), m_vBannedAddress.end(), netAdr2.GetIP(true)) != m_vBannedAddress.end())
	{
		return true;
	}

	// Check if net console has reached maximum number of attempts and add to ban vector.
	if (pData->m_nFailedAttempts >= CVar_sv_rcon_maxfailures->GetInt() || pData->m_nIgnoredMessage >= CVar_sv_rcon_maxignores->GetInt())
	{
		// Don't add whitelisted address to ban vector.
		if (std::strcmp(netAdr2.GetIP(true).c_str(), CVar_sv_rcon_whitelist_address->GetString()) == 0)
		{
			pData->m_nFailedAttempts = 0;
			pData->m_nIgnoredMessage = 0;
			return false;
		}

		spdlog::info("Banned '{:s}' for RCON hacking attempts", netAdr2.GetIPAndPort());
		m_vBannedAddress.push_back(netAdr2.GetIP(true));
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: close specific connection
//-----------------------------------------------------------------------------
void CRConServer::CloseConnection(void) // NETMGR
{
	CConnectedNetConsoleData* pData = m_pSocket->GetAcceptedSocketData(m_nConnIndex);
	if (pData->m_bAuthorized)
	{
		// Inform server owner when authenticated connection has been closed.
		CNetAdr2 netAdr2 = m_pSocket->GetAcceptedSocketAddress(m_nConnIndex);
		spdlog::info("Net console '{:s}' closed RCON connection", netAdr2.GetIPAndPort());
	}
	m_pSocket->CloseAcceptedSocket(m_nConnIndex);
}

//-----------------------------------------------------------------------------
// Purpose: close all connections except for authenticated
//-----------------------------------------------------------------------------
void CRConServer::CloseNonAuthConnection(void)
{
	int nCount = m_pSocket->GetAcceptedSocketCount();
	for (int i = nCount - 1; i >= 0; i--)
	{
		CConnectedNetConsoleData* pData = m_pSocket->GetAcceptedSocketData(i);

		if (!pData->m_bAuthorized)
		{
			m_pSocket->CloseAcceptedSocket(i);
		}
	}
}
///////////////////////////////////////////////////////////////////////////////
CRConServer* g_pRConServer = new CRConServer();
CRConServer* RCONServer()
{
	return g_pRConServer;
}
