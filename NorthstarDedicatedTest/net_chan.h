//=============================================================================//
//
// Purpose:
//
//=============================================================================//

#ifndef NET_CHAN_H
#define NET_CHAN_H

#include "bitbuf.h"
#include "valve_utl.h"
#include "serverauthentication.h"

#define NET_FRAMES_BACKUP 128
#define NAT_CHANNEL_MAXNAME 32
#define NET_FRAMES_MASK (NET_FRAMES_BACKUP - 1)

#define NET_FRAMES_BACKUP 128
#define FLOW_OUTGOING 0
#define FLOW_INCOMING 1
#define MAX_FLOWS 2 // in & out

//-----------------------------------------------------------------------------
// Purpose: forward declarations
//-----------------------------------------------------------------------------
class CClient;

//-----------------------------------------------------------------------------
struct netframe_t
{
	float one;
	float two;
	float three;
	float four;
	float five;
	float six;
};

//-----------------------------------------------------------------------------
struct netflow_t
{
	float nextcompute;
	float avgbytespersec;
	float avgpacketspersec;
	float avgloss;
	float avgchoke;
	float avglatency;
	float latency;
	int64_t totalpackets;
	int64_t totalbytes;
	netframe_t frames[NET_FRAMES_BACKUP];
	netframe_t current_frame;
};

//-----------------------------------------------------------------------------
struct dataFragments_t
{
	char* data;
	int64_t block_size;
	bool m_bIsCompressed;
	uint8_t gap11[7];
	int64_t m_nRawSize;
	bool m_bFirstFragment;
	bool m_bLastFragment;
	bool m_bIsOutbound;
	int transferID;
	int m_nTransferSize;
	int m_nCurrentOffset;
};

struct VecNetDataFragments
{
	void** items;
	__int64 m_nAllocationCount;
	__int64 m_nGrowSize;
	int m_Size;
	int padding_;
};

struct CNetMessage
{
	void* iNetMessageVTable;
	int m_nGroup;
	bool m_bReliable;
	char padding[3];
	void* m_NetChannel;
};


struct VecNetMessages
{
	CNetMessage** items;
	__int64 m_nAllocationCount;
	__int64 m_nGrowSize;
	int m_Size;
	int padding_;
};


//-----------------------------------------------------------------------------
enum EBufType
{
	BUF_RELIABLE = 0,
	BUF_UNRELIABLE,
	BUF_VOICE
};

class INetChannelHandler
{
	void* __vftable;
};

typedef enum
{
	NA_NULL = 0,
	NA_LOOPBACK,
	NA_IP,
} netadrtype_t;

class netadr_t
{
  public:
	inline netadrtype_t GetType(void) const
	{
		return this->type;
	}
	inline std::string GetAddress(void) const
	{
		char szAdr[INET6_ADDRSTRLEN] {};
		inet_ntop(AF_INET6, &this->adr, szAdr, INET6_ADDRSTRLEN);

		return szAdr;
	}
	inline uint16_t GetPort(void) const
	{
		return this->port;
	}
	inline bool IsReliable(void) const
	{
		return this->reliable;
	}
	netadrtype_t type;
	IN6_ADDR adr;
	uint16_t port;
	bool field_16;
	bool reliable;
};

//-----------------------------------------------------------------------------
class CNetChan // [WARNING: SOME MEMBERS ARE WRONG AS THIS IS TAKEN FROM R5R, 'remote_address' DOES ALIGN].
{
  public:
	std::string GetName(void) const;
	std::string GetAddress(void) const;
	int GetDataRate(void) const;
	int GetBufferSize(void) const;

	float GetLatency(int flow) const;
	float GetAvgChoke(int flow) const;
	float GetAvgLatency(int flow) const;
	float GetAvgLoss(int flow) const;
	float GetAvgPackets(int flow) const;
	float GetAvgData(int flow) const;

	int GetTotalData(int flow) const;
	int GetTotalPackets(int flow) const;
	int GetSequenceNr(int flow) const;

	float GetTimeoutSeconds(void) const;
	double GetConnectTime(void) const;
	int GetSocket(void) const;

	bool IsOverflowed(void) const;
	void Clear(bool bStopProcessing);
	//-----------------------------------------------------------------------------
  private:
	bool m_bProcessingMessages;
	bool m_bShouldDelete;
	bool m_bStopProcessing;
	bool shutting_down;
	int m_nOutSequenceNr;
	int m_nInSequenceNr;
	int m_nOutSequenceNrAck;
	int m_nChokedPackets;
	int unknown_challenge_var;
	int m_nLastRecvFlags;
	RTL_SRWLOCK LOCK;
	BFWrite m_StreamReliable;
	CUtlMemory m_ReliableDataBuffer;
	BFWrite m_StreamUnreliable;
	CUtlMemory m_UnreliableDataBuffer;
	struct BFWrite m_StreamVoice;
	CUtlMemory m_VoiceDataBuffer;
	int m_Socket;
	int m_MaxReliablePayloadSize;
	double last_received;
	double connect_time;
	unsigned int m_Rate;
	int padding_maybe;
	double m_fClearTime;
	VecNetDataFragments m_WaitingList;
	dataFragments_t m_ReceiveList;
	int m_nSubOutFragmentsAck;
	int m_nSubInFragments;
	int m_nNonceHost;
	unsigned int m_nNonceRemote;
	bool m_bReceivedRemoteNonce;
	bool m_bInReliableState;
	bool m_bPendingRemoteNonceAck;
	unsigned int m_nSubOutSequenceNr;
	int m_nLastRecvNonce;
	bool m_bUseCompression;
	unsigned int dword168;
	float m_Timeout;
	INetChannelHandler* m_MessageHandler;
	VecNetMessages m_NetMessages;
	unsigned int qword198;
	int m_nQueuedPackets;
	float m_flRemoteFrameTime;
	float m_flRemoteFrameTimeStdDeviation;
	unsigned __int8 m_bUnkTickBool;
	int m_nMaxRoutablePayloadSize;
	int m_nSplitPacketSequence;
	__int64 m_StreamSendBuffer;
	BFWrite m_StreamSend;
	unsigned __int8 m_bInMatch_maybe;
	netflow_t m_DataFlow[2];
	int m_nLifetimePacketsDropped;
	int m_nSessionPacketsDropped;
	int m_nSequencesSkipped_MAYBE;
	int m_nSessionRecvs;
	unsigned int m_nLiftimeRecvs;
	char TODO[6972];
	char m_Name[35];
	unsigned __int8 m_bRetrySendLong;
	netadr_t remote_address;
};

#endif // NET_CHAN_H
