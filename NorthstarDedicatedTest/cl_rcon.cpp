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
#include "igameserverdata.h"

//-----------------------------------------------------------------------------
// Purpose: NETCON systems init
//-----------------------------------------------------------------------------
void CRConClient::Init(void)
{
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
		spdlog::info("Connection to RCON server '{}' failed: (SOCKET_ERROR)", m_pNetAdr2->GetIPAndPort());
		return false;
	}
	spdlog::info("Connected to: {}", m_pNetAdr2->GetIPAndPort().c_str());

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
		spdlog::info("Connection to RCON server '{}' failed: (SOCKET_ERROR)", m_pNetAdr2->GetIPAndPort());
		return false;
	}
	spdlog::info("Connected to: {}", m_pNetAdr2->GetIPAndPort().c_str());

	m_bConnEstablished = true;
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: disconnect from current session
//-----------------------------------------------------------------------------
void CRConClient::Disconnect(void)
{
	::closesocket(m_pSocket->GetAcceptedSocketHandle(0));
	m_bConnEstablished = false;
}

//-----------------------------------------------------------------------------
// Purpose: send message
// Input  : *svMessage - 
//-----------------------------------------------------------------------------
void CRConClient::Send(const std::string& svMessage) const
{
	int nSendResult = ::send(m_pSocket->GetAcceptedSocketData(0)->m_hSocket, svMessage.c_str(), svMessage.size(), MSG_NOSIGNAL);
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
	static char szRecvBuf[MAX_NETCONSOLE_INPUT_LEN]{};

	{//////////////////////////////////////////////
		int nPendingLen = ::recv(m_pSocket->GetAcceptedSocketData(0)->m_hSocket, szRecvBuf, sizeof(szRecvBuf), MSG_PEEK);
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
	}//////////////////////////////////////////////

	u_long nReadLen; // Find out how much we have to read.
	::ioctlsocket(m_pSocket->GetAcceptedSocketData(0)->m_hSocket, FIONREAD, &nReadLen);

	while (nReadLen > 0)
	{
		memset(szRecvBuf, '\0', sizeof(szRecvBuf));
		int nRecvLen = ::recv(m_pSocket->GetAcceptedSocketData(0)->m_hSocket, szRecvBuf, MIN(sizeof(szRecvBuf), nReadLen), MSG_NOSIGNAL);

		if (nRecvLen == 0 && m_bConnEstablished) // Socket was closed.
		{
			this->Disconnect();
			spdlog::info("Server closed RCON connection");
			break;
		}
		if (nRecvLen < 0 && !m_pSocket->IsSocketBlocking())
		{
			break;
		}

		nReadLen -= nRecvLen; // Process what we've got.
		this->ProcessBuffer(szRecvBuf, nRecvLen);
	}
}

//-----------------------------------------------------------------------------
// Purpose: handles input response buffer
// Input  : *pszIn - 
//			nRecvLen - 
//-----------------------------------------------------------------------------
void CRConClient::ProcessBuffer(const char* pszIn, int nRecvLen) const
{
	int nCharsInRespondBuffer = 0;
	char szInputRespondBuffer[MAX_NETCONSOLE_INPUT_LEN]{};

	while (nRecvLen)
	{
		switch (*pszIn)
		{
		case '\r':
		{
			if (nCharsInRespondBuffer)
			{
				sv_rcon::response sv_response = this->Deserialize(szInputRespondBuffer);
				this->ProcessMessage(sv_response);
			}
			nCharsInRespondBuffer = 0;
			break;
		}

		default:
		{
			if (nCharsInRespondBuffer < MAX_NETCONSOLE_INPUT_LEN - 1)
			{
				szInputRespondBuffer[nCharsInRespondBuffer++] = *pszIn;
			}
			break;
		}
		}
		pszIn++;
		nRecvLen--;
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
		spdlog::info("{}", svOut.c_str());
		break;
	}
	case sv_rcon::response_t::SERVERDATA_RESPONSE_CONSOLE_LOG:
	{
		// !TODO: Network the enum to differentiate script/engine logs.
		svOut.erase(std::remove(svOut.begin(), svOut.end(), '\n'), svOut.end());
		spdlog::info("{}", svOut.c_str());
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
	return cl_request.SerializeAsString().append("\r");
}

//-----------------------------------------------------------------------------
// Purpose: de-serializes input
// Input  : *svBuf - 
// Output : de-serialized object
//-----------------------------------------------------------------------------
sv_rcon::response CRConClient::Deserialize(const std::string& svBuf) const
{
	sv_rcon::response sv_response;
	sv_response.ParseFromArray(svBuf.c_str(), static_cast<int>(svBuf.size()));

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
