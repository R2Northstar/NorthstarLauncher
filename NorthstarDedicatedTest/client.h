#pragma once
#include "protocol.h"
#include "net_chan.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CServer;
class CClient;

struct IClientMessageHandler
{
	void* __vftable;
};

///////////////////////////////////////////////////////////////////////////////
extern CClient* g_pClient;

class CClient : INetChannelHandler, IClientMessageHandler
{
  public:
	CClient* GetClient(int nIndex) const;
	uint16_t GetHandle(void) const;
	uint32_t GetUserID(void) const;
	uint64_t GetOriginID(void) const;
	SIGNONSTATE GetSignonState(void) const;
	PERSISTENCE GetPersistenceState(void) const;
	CNetChan* GetNetChan(void) const;
	const char* GetServerName(void) const;
	const char* GetClientName(void) const;
	void SetHandle(uint16_t nHandle);
	void SetUserID(uint32_t nUserID);
	void SetOriginID(uint64_t nOriginID);
	void SetSignonState(SIGNONSTATE nSignonState);
	void SetPersistenceState(PERSISTENCE nPersistenceState);
	void SetNetChan(CNetChan* pNetChan);
	bool IsConnected(void) const;
	bool IsSpawned(void) const;
	bool IsActive(void) const;
	bool IsPersistenceAvailable(void) const;
	bool IsPersistenceReady(void) const;
	bool IsFakeClient(void) const;
	bool IsHumanPlayer(void) const;
	bool Connect(const char* szName, void* pNetChannel, bool bFakePlayer, void* a5, char* szMessage, int nMessageSize);
	static bool
	VConnect(CClient* pClient, const char* szName, void* pNetChannel, bool bFakePlayer, void* a5, char* szMessage, int nMessageSize);
	void Clear(void);
	static void VClear(CClient* pBaseClient);

  private:
	uint32_t m_nUserID;
	uint16_t m_nHandle;
	char m_szServerName[64];
	int64_t m_nReputation;
	char pad_0014[182];
	char m_szClientName[64];
	char pad_0015[252];
	void* m_ConVars; // [KeyValues*]
	char pad_0368[8];
	CServer* m_pServer;
	char pad_0378[32];
	CNetChan* m_NetChannel;
	char pad_03A8[8];
	SIGNONSTATE m_nSignonState;
	int32_t m_nDeltaTick;
	uint64_t m_nOriginID;
	int32_t m_nStringTableAckTick;
	int32_t m_nSignonTick;
	char pad_03C0[460];
	bool m_bFakePlayer;
	bool m_bReceivedPacket;
	bool m_bLowViolence;
	bool m_bFullyAuthenticated;
	char pad_05A4[24];
	PERSISTENCE m_nPersistenceState;
	char pad_05C0[184964];
};

void InitialiseBaseClient(HMODULE baseAddress);