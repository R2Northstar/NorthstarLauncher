#include "offline_persistence.h"

#include "server/auth/serverauthentication.h"
#include "engine/r2engine.h"

#include <shlobj_core.h>
#include <fstream>
#include <filesystem>
#include <cassert>
#include <client/r2client.h>

OfflinePersistence* g_pOfflinePersistence;

constexpr auto BASE_CONNECT_PAYLOAD_LENGTH = 21;

void OfflinePersistence::ReadAndEnqueuePDataFromConnectPacket(const unsigned char* packetData, size_t packetSize)
{

	// Protocol addition:
	// 8 bytes: UID (uint64_t)
	// 2 bytes: Length of pData (ushort)
	// <length> bytes: pData
	uint64_t uid {};
	uint16_t pDataSize {};
	char* pDataBuffer {};

	size_t i = BASE_CONNECT_PAYLOAD_LENGTH;

// Safe read
#define READ(x)                                                                                                                            \
	if (i + sizeof(x) <= packetSize)                                                                                                       \
	{                                                                                                                                      \
		memcpy(&x, &packetData[i], sizeof(x));                                                                                             \
		i += sizeof(x);                                                                                                                    \
	}

	READ(uid);
	READ(pDataSize);

	if (i + pDataSize >= packetSize)
	{
		pDataBuffer = new char[pDataSize];
		memcpy(pDataBuffer, &packetData[i], pDataSize);
		i += pDataSize;
	}
	else
	{
		pDataSize = 0;
	}

#undef READ

	// Erase previous auth data if any
	const auto uidStr = std::to_string(uid);
	g_pServerAuthentication->m_RemoteAuthenticationData.erase(uidStr);

	// Create the new one to put it on server auth, it will get dequeued on next connection (which is this one)
	RemoteAuthData newAuthData {};
	strncpy_s(newAuthData.uid, sizeof(newAuthData.uid), uidStr.c_str(), sizeof(newAuthData.uid) - 1);
	newAuthData.pdataSize = pDataSize;
	newAuthData.pdata = pDataBuffer;

	g_pServerAuthentication->m_RemoteAuthenticationData.insert(std::make_pair(uidStr, newAuthData));
}

void OfflinePersistence::AppendPDataToConnectPacket(OUT const char*& data, OUT int& length, OUT std::allocator<char>& allocator)
{
	// Protocol addition:
	// 8 bytes: UID (uint64_t)
	// 2 bytes: Length of pData (ushort)
	// <length> bytes: pData

	const uint64_t uid = g_pLocalPlayerUserID ? std::stoull(g_pLocalPlayerUserID) : 0;
	const auto maxPersistenceBuffer = allocator.allocate(PERSISTENCE_MAX_SIZE);
	ZeroMemory(maxPersistenceBuffer, PERSISTENCE_MAX_SIZE);

	size_t persistenceLength {};
	ExportOfflinePersistentData(maxPersistenceBuffer, persistenceLength);

	const auto additionalLength = 8 // uid (uint64_t)
								  + 2 // length prefix for pdata (ushort)
								  + persistenceLength;

	const auto newMessageLength = length + additionalLength;

	const auto newMessageBuffer = allocator.allocate(newMessageLength);
	ZeroMemory(newMessageBuffer, newMessageLength);

	size_t i = 0;

	// Copy original message
	memcpy(&newMessageBuffer[i], data, length);
	i += length;

	// Copy UID
	memcpy(&newMessageBuffer[i], &uid, sizeof(uid));
	i += sizeof(uid);

	// Write length
	assert(persistenceLength <= std::numeric_limits<uint16_t>().max()); // The things i'd do not to write USHRT_MAX
	const auto persistenceLengthU16 = static_cast<uint16_t>(persistenceLength);
	memcpy(&newMessageBuffer[i], &persistenceLengthU16, sizeof(persistenceLengthU16));
	i += sizeof(persistenceLengthU16);

	// Copy persistence
	memcpy(&newMessageBuffer[i], maxPersistenceBuffer, persistenceLength);
	i += persistenceLength;

	data = newMessageBuffer;
	length = i;
}

void OfflinePersistence::ExportOfflinePersistentData(OUT char* buffer, OUT size_t& len)
{
	const auto path = GetOfflinePersistentDataPath();
	std::ifstream localData(path, std::ios::binary | std::ios::in);
	if (localData.is_open())
	{
		ZeroMemory(buffer, PERSISTENCE_MAX_SIZE);
		localData.read(buffer, PERSISTENCE_MAX_SIZE);
		const auto streamPosition = localData.gcount();
		if (streamPosition != -1)
		{
			len = streamPosition;
		}
		else
		{
			len = PERSISTENCE_MAX_SIZE;
		}
	}
}

void OfflinePersistence::ReadOfflinePersistentData(CBaseClient* pPlayer)
{
	assert(g_pServerAuthentication->IsLocalPlayer(pPlayer));

	const auto path = GetOfflinePersistentDataPath();
	std::ifstream localData(path, std::ios::binary | std::ios::in);
	if (localData.is_open())
	{
		ZeroMemory(pPlayer->m_PersistenceBuffer, ARRAYSIZE(pPlayer->m_PersistenceBuffer));
		localData.read(pPlayer->m_PersistenceBuffer, ARRAYSIZE(pPlayer->m_PersistenceBuffer));
	}
}

void OfflinePersistence::WriteOfflinePersistentData(CBaseClient* pPlayer)
{
	assert(g_pServerAuthentication->IsLocalPlayer(pPlayer));

	WriteOfflinePersistentData(pPlayer->m_PersistenceBuffer, g_pServerAuthentication->m_PlayerAuthenticationData[pPlayer].pdataSize);
}

void OfflinePersistence::WriteOfflinePersistentData(const char* buffer, size_t size)
{
	if (size > 0) // Prevent a zero-size write from clearing the persistent data entirely
	{
		const auto path = GetOfflinePersistentDataPath();
		const auto dir = path.parent_path();
		std::filesystem::create_directory(dir);

		std::ofstream localData(path, std::ios::binary | std::ios::out);
		if (localData.is_open())
		{
			localData.write(buffer, size);
		}
	}
}

std::filesystem::path OfflinePersistence::GetOfflinePersistentDataPath()
{
	const auto filename = std::format("persistent.pdata");

	// TODO: Where is the best place to put persistent NS files? I vote for AppData but is another place already defined for this?
	PWSTR appData = NULL;
	if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, NULL, &appData) == S_OK)
	{
		char dest[MAX_PATH];
		wcstombs(dest, appData, MAX_PATH);

		// Create a folder for northstar
		std::filesystem::path appDataPath(dest);

		return appDataPath / "Northstar" / filename;
	}

	return std::filesystem::path(filename);
}

const uint64_t __fastcall OfflinePersistence::h_NET_SendPacket(
	void* chan, int _socketIndex, const netadr_s* address, const char* data, int length, __int64 arg_28, char bCompressed, int a8, char a9)
{
	if (g_pServerAuthentication->Cvar_ns_auth_allow_insecure_write->GetBool())
	{
		bool isConnectPacket = _socketIndex == 0 && length == BASE_CONNECT_PAYLOAD_LENGTH && data[4] == 'H';
		if (isConnectPacket)
		{
			std::allocator<char> allocator;

			g_pOfflinePersistence->AppendPDataToConnectPacket(data, length, allocator);

			bCompressed = length > 1024; // This will very likely occur. Since single burst transmissions are <1228, we compress above ~1000

			const auto result = NET_SendPacket(chan, _socketIndex, address, data, length, arg_28, bCompressed, a8, a9);

			return result;
		}
	}

	return NET_SendPacket(chan, _socketIndex, address, data, length, arg_28, bCompressed, a8, a9);
}

void (*OfflinePersistence::o_pPacketHandler_HandleConnect)(void* packetHandler, int source, netpacket_s* packet, __int64 a4, bool a5) =
	nullptr;
void OfflinePersistence::h_PacketHandler_HandleConnect(void* packetHandler, int source, netpacket_s* packet, __int64 a4, bool a5)
{
	if (g_pServerAuthentication->Cvar_ns_auth_allow_insecure_write->GetBool())
	{
		if (packet->size > BASE_CONNECT_PAYLOAD_LENGTH) // Extra connect data is used to supply persistent data upon connection
		{
			g_pOfflinePersistence->ReadAndEnqueuePDataFromConnectPacket(packet->data, packet->size);
		}
	}

	o_pPacketHandler_HandleConnect(packetHandler, source, packet, a4, a5);
}

// Upon local client disconnection, write all persistent data down to disk
// TODO: Write it when it's changed instead? (after a call to SetPersistentVar ?)
static void (*o_pCClientState__Disconnect)(CClientState* self, char unk) = nullptr;
void h_CClientState__Disconnect(CClientState* self, char unk)
{
	if (g_pServerAuthentication->Cvar_ns_auth_allow_insecure_write->GetBool() && *self->persistentData)
	{
		g_pOfflinePersistence->WriteOfflinePersistentData(reinterpret_cast<const char*>(self->persistentData), PERSISTENCE_MAX_SIZE);
	}

	o_pCClientState__Disconnect(self, unk);
}

ON_DLL_LOAD("engine.dll", OfflinePersistenceCtor, (CModule module))
{
	g_pOfflinePersistence = new OfflinePersistence(module);
}

OfflinePersistence::OfflinePersistence(CModule module)
{
	HookAttach(&(PVOID&)NET_SendPacket, (PVOID)h_NET_SendPacket);

	o_pPacketHandler_HandleConnect = module.Offset(0x1183A0).RCast<decltype(o_pPacketHandler_HandleConnect)>();
	HookAttach(&(PVOID&)o_pPacketHandler_HandleConnect, (PVOID)h_PacketHandler_HandleConnect);

	o_pCClientState__Disconnect = module.Offset(0x8DC50).RCast<decltype(o_pCClientState__Disconnect)>();
	HookAttach(&(PVOID&)o_pCClientState__Disconnect, (PVOID)h_CClientState__Disconnect);
}
