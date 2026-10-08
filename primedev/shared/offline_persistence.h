#pragma once

class OfflinePersistence
{
public:
	void ExportOfflinePersistentData(OUT char* buffer, OUT size_t& len);
	void WriteOfflinePersistentData(const char* buffer, size_t length);
	void WriteOfflinePersistentData(class CBaseClient* pPlayer);
	void ReadOfflinePersistentData(class CBaseClient* pPlayer);

	OfflinePersistence(CModule engineModule);

private:
	std::filesystem::path GetOfflinePersistentDataPath();

	void ReadAndEnqueuePDataFromConnectPacket(const unsigned char* packetData, size_t packetSize);
	void AppendPDataToConnectPacket(OUT const char*& data, OUT int& length, OUT std::allocator<char>& allocator);

	// Hooks
	static const uint64_t __fastcall h_NET_SendPacket(
		void* chan,
		int _socketIndex,
		const struct netadr_s* address,
		const char* data,
		int length,
		__int64 arg_28,
		char bCompressed,
		int a8,
		char a9);

	static void h_PacketHandler_HandleConnect(void* packetHandler, int source, struct netpacket_s* packet, __int64 a4, bool a5);

	static const uint64_t(__fastcall* OfflinePersistence::o_pNET_SendPacket)(
		void* chan,
		int _socketIndex,
		const struct netadr_s* address,
		const char* data,
		int length,
		__int64 arg_28,
		char bCompressed,
		int a8,
		char a9);

	static void (*OfflinePersistence::o_pPacketHandler_HandleConnect)(
		void* packetHandler, int source, struct netpacket_s* packet, __int64 a4, bool a5);
};

extern OfflinePersistence* g_pOfflinePersistence;
