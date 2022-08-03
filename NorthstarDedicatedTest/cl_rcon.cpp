//===========================================================================//
//
// Purpose: Implementation of the rcon client.
//
//===========================================================================//

#include "pch.h"
#include "convar.h"
#include "concommand.h"
#include "cvar.h"
#include "rcon_shared.h"
#include "sv_rcon.pb.h"
#include "cl_rcon.pb.h"
#include "cl_rcon.h"
// #include "net.h"
#include "igameserverdata.h"

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CRConClient::CRConClient() : m_bInitialized(false), m_bConnEstablished(false)
{
	m_pNetAdr2 = new CNetAdr2("localhost", "37015");
	m_pSocket = new CSocketCreator();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CRConClient::~CRConClient(void)
{
	delete m_pNetAdr2;
	delete m_pSocket;
}

//-----------------------------------------------------------------------------
// Purpose: NETCON systems init
//-----------------------------------------------------------------------------
void CRConClient::Init(void)
{
	if (!m_bInitialized)
	{
		this->SetPassword(CVar_rcon_password->GetString());
	}
	m_bInitialized = true;
}

//-----------------------------------------------------------------------------
// Purpose: NETCON systems shutdown
//-----------------------------------------------------------------------------
void CRConClient::Shutdown(void)
{
	if (m_bConnEstablished)
	{
		this->Disconnect();
	}
}

//-----------------------------------------------------------------------------
// Purpose: changes the password
// Input  : *pszPassword -
// Output : true on success, false otherwise
//-----------------------------------------------------------------------------
bool CRConClient::SetPassword(const char* pszPassword)
{
	if (std::strlen(pszPassword) < 8)
	{
		if (std::strlen(pszPassword) > 0)
		{
			spdlog::info("Remote server access requires a password of at least 8 characters");
		}
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: client rcon main processing loop
//-----------------------------------------------------------------------------
void CRConClient::RunFrame(void)
{
	if (m_bInitialized && m_bConnEstablished)
	{
		this->Recv();
	}
}

//-----------------------------------------------------------------------------
// Purpose: connect to address and port stored in 'rcon_address' cvar
// Output : true if connection succeeds, false otherwise
//-----------------------------------------------------------------------------
bool CRConClient::Connect(void)
{
	if (strlen(CVar_rcon_address->GetString()) > 0)
	{
		// Default is [127.0.0.1]:37015
		m_pNetAdr2->SetIPAndPort(CVar_rcon_address->GetString());
	}

	if (m_pSocket->ConnectSocket(*m_pNetAdr2, true) == SOCKET_ERROR)
	{
		spdlog::info("Connection to RCON server '{:s}' failed: (SOCKET_ERROR)", m_pNetAdr2->GetIPAndPort());
		return false;
	}
	spdlog::info("Connected to: {:s}", m_pNetAdr2->GetIPAndPort());

	m_bConnEstablished = true;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: connect to specified address and port
// Input  : *svInAdr -
//			*svInPort -
// Output : true if connection succeeds, false otherwise
//-----------------------------------------------------------------------------
bool CRConClient::Connect(const std::string& svInAdr, const std::string& svInPort)
{
	if (svInAdr.size() > 0 && svInPort.size() > 0)
	{
		// Default is [127.0.0.1]:37015
		m_pNetAdr2->SetIPAndPort(svInAdr, svInPort);
	}

	if (m_pSocket->ConnectSocket(*m_pNetAdr2, true) == SOCKET_ERROR)
	{
		spdlog::info("Connection to RCON server '{:s}' failed: (SOCKET_ERROR)", m_pNetAdr2->GetIPAndPort());
		return false;
	}
	spdlog::info("Connected to: {:s}", m_pNetAdr2->GetIPAndPort());

	m_bConnEstablished = true;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: disconnect from current session
//-----------------------------------------------------------------------------
void CRConClient::Disconnect(void)
{
	m_pSocket->CloseAcceptedSocket(0);
	m_bConnEstablished = false;
}

//-----------------------------------------------------------------------------
// Purpose: send message
// Input  : *svMessage -
//-----------------------------------------------------------------------------
void CRConClient::Send(const std::string& svMessage) const
{
	std::ostringstream ssSendBuf;

	ssSendBuf << static_cast<uint8_t>(static_cast<int>(svMessage.size()) >> 24);
	ssSendBuf << static_cast<uint8_t>(static_cast<int>(svMessage.size()) >> 16);
	ssSendBuf << static_cast<uint8_t>(static_cast<int>(svMessage.size()) >> 8);
	ssSendBuf << static_cast<uint8_t>(static_cast<int>(svMessage.size()));
	ssSendBuf << svMessage;

	int nSendResult = ::send(
		m_pSocket->GetAcceptedSocketData(0)->m_hSocket, ssSendBuf.str().data(), static_cast<int>(ssSendBuf.str().size()), MSG_NOSIGNAL);
	if (nSendResult == SOCKET_ERROR)
	{
		spdlog::info("Failed to send RCON message: (SOCKET_ERROR)");
	}
}

//-----------------------------------------------------------------------------
// Purpose: receive message
//-----------------------------------------------------------------------------
void CRConClient::Recv(void)
{
	static char szRecvBuf[1024];
	CConnectedNetConsoleData* pData = m_pSocket->GetAcceptedSocketData(0);

	{ //////////////////////////////////////////////
		int nPendingLen = ::recv(pData->m_hSocket, szRecvBuf, sizeof(char), MSG_PEEK);
		if (nPendingLen == SOCKET_ERROR && m_pSocket->IsSocketBlocking())
		{
			return;
		}
		if (nPendingLen <= 0 && m_bConnEstablished) // EOF or error.
		{
			this->Disconnect();
			spdlog::info("Server closed RCON connection");
			return;
		}
	} //////////////////////////////////////////////

	u_long nReadLen; // Find out how much we have to read.
	::ioctlsocket(pData->m_hSocket, FIONREAD, &nReadLen);

	while (nReadLen > 0)
	{
		int nRecvLen = ::recv(pData->m_hSocket, szRecvBuf, MIN(sizeof(szRecvBuf), nReadLen), MSG_NOSIGNAL);
		if (nRecvLen == 0 && m_bConnEstablished) // Socket was closed.
		{
			this->Disconnect();
			spdlog::info("Server closed RCON connection");
			break;
		}
		if (nRecvLen < 0 && !m_pSocket->IsSocketBlocking())
		{
			spdlog::error("RCON Cmd: recv error");
			break;
		}

		nReadLen -= nRecvLen; // Process what we've got.
		this->ProcessBuffer(szRecvBuf, nRecvLen, pData);
	}
}

//-----------------------------------------------------------------------------
// Purpose: parses input response buffer using length-prefix framing
// Input  : *pszIn -
//			nRecvLen -
//			*pData -
//-----------------------------------------------------------------------------
void CRConClient::ProcessBuffer(const char* pRecvBuf, int nRecvLen, CConnectedNetConsoleData* pData)
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

			if (pData->m_nPayloadLen < 0)
			{
				spdlog::error("RCON Cmd: sync error ({:d})", pData->m_nPayloadLen);
				this->Disconnect(); // Out of sync (irrecoverable).

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
// Input  : *sv_response -
//-----------------------------------------------------------------------------
void CRConClient::ProcessMessage(const sv_rcon::response& sv_response) const
{
	std::string svOut = sv_response.responsebuf();

	switch (sv_response.responsetype())
	{
	case sv_rcon::response_t::SERVERDATA_RESPONSE_AUTH:
	{
		svOut.erase(std::remove(svOut.begin(), svOut.end(), '\n'), svOut.end());
		spdlog::info("{:s}", svOut);
		break;
	}
	case sv_rcon::response_t::SERVERDATA_RESPONSE_CONSOLE_LOG:
	{
		// !TODO: Network the enum to differentiate script/engine logs.
		svOut.erase(std::remove(svOut.begin(), svOut.end(), '\n'), svOut.end());
		spdlog::info("{:s}", svOut);
		break;
	}
	default:
	{
		break;
	}
	}
}

//-----------------------------------------------------------------------------
// Purpose: serializes input
// Input  : *svReqBuf -
//			*svReqVal -
//			request_t -
// Output : serialized results as string
//-----------------------------------------------------------------------------
std::string CRConClient::Serialize(const std::string& svReqBuf, const std::string& svReqVal, cl_rcon::request_t request_t) const
{
	cl_rcon::request cl_request;

	cl_request.set_requestid(-1);
	cl_request.set_requesttype(request_t);

	switch (request_t)
	{
	case cl_rcon::request_t::SERVERDATA_REQUEST_SETVALUE:
	case cl_rcon::request_t::SERVERDATA_REQUEST_AUTH:
	{
		cl_request.set_requestbuf(svReqBuf);
		cl_request.set_requestval(svReqVal);
		break;
	}
	case cl_rcon::request_t::SERVERDATA_REQUEST_EXECCOMMAND:
	{
		cl_request.set_requestbuf(svReqBuf);
		break;
	}
	}
	return cl_request.SerializeAsString();
}

//-----------------------------------------------------------------------------
// Purpose: de-serializes input
// Input  : *svBuf -
// Output : de-serialized object
//-----------------------------------------------------------------------------
sv_rcon::response CRConClient::Deserialize(const std::string& svBuf) const
{
	sv_rcon::response sv_response;
	sv_response.ParseFromArray(svBuf.data(), static_cast<int>(svBuf.size()));

	return sv_response;
}

//-----------------------------------------------------------------------------
// Purpose: checks if client rcon is initialized
// Output : true if initialized, false otherwise
//-----------------------------------------------------------------------------
bool CRConClient::IsInitialized(void) const
{
	return m_bInitialized;
}

//-----------------------------------------------------------------------------
// Purpose: checks if client rcon is connected
// Output : true if connected, false otherwise
//-----------------------------------------------------------------------------
bool CRConClient::IsConnected(void) const
{
	return m_bConnEstablished;
}
///////////////////////////////////////////////////////////////////////////////
CRConClient* g_pRConClient = new CRConClient();
CRConClient* RCONClient()
{
	return g_pRConClient;
}