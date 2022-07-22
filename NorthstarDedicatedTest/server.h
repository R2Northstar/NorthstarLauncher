#pragma once
#include "net_chan.h"
#include "valve_utl.h"
#include "stringtable.h"
#include "stringtable.h"
#include "gameutils.h" // For 'server_state_t' (this should be moved to this header).

#define MAX_MAP_NAME 64

struct user_creds_s
{
	netadr_t m_nAddr;
	int32_t m_nProtocolVer;
	int32_t m_nchallenge;
	uint8_t gap2[8];
	uint64_t m_nNucleusID;
	uint8_t* m_nUserID;
};

class IServer
{
	IServer* m_pVTable;
};

class CServer : public IServer
{
  public:
	int GetTick(void) const
	{
		return m_nTickCount;
	}
#ifndef CLIENT_DLL // Only the connectionless packet handler is implemented on the client via the IServer base class.
	int GetNumHumanPlayers(void) const;
	int GetNumFakeClients(void) const;
	const char* GetMapName(void) const
	{
		return m_szMapname;
	}
	const char* GetMapGroupName(void) const
	{
		return m_szMapGroupName;
	}
	int GetNumClasses(void) const
	{
		return m_nServerClasses;
	}
	int GetClassBits(void) const
	{
		return m_nServerClassBits;
	}
	bool IsActive(void) const
	{
		return m_State >= server_state_t::ss_active;
	}
	bool IsLoading(void) const
	{
		return m_State == server_state_t::ss_loading;
	}
	bool IsDedicated(void) const
	{
		//return g_bDedicated;
	}
	static CClient* Authenticate(CServer* pServer, user_creds_s* pInpacket);
#endif // !CLIENT_DLL

  private:
	server_state_t m_State;
	int m_Socket;
	int m_nTickCount;
	bool m_bResetMaxTeams;
	char m_szMapname[64];
	char m_szMapGroupName[64];
	char m_Password[32];
	uint32_t worldmapCRC;
	uint32_t clientDllCRC;
	void* unkData;
	CNetworkStringTableContainer* m_StringTables;
	CNetworkStringTable* m_pInstanceBaselineTable;
	CNetworkStringTable* m_pLightStyleTable;
	CNetworkStringTable* m_pUserInfoTable;
	CNetworkStringTable* m_pServerQueryTable;
	bool m_bReplay;
	bool m_bUpdateFrame;
	bool m_bUseReputation;
	bool m_bSimulating;
	int m_nPad;
	BFWrite m_Signon;
	CUtlMemory m_SignonBuffer;
	int m_nServerClasses;
	int m_nServerClassBits;
	char m_szConDetails[64];
	char m_szHostInfo[128];
	char m_PadArray[303824];
};
extern CServer* g_pServer;

void InitialiseBaseServer(HMODULE baseAddress);