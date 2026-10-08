#include "lan.h"

#include "server/serverpresence.h"
#include "engine/r2engine.h"
#include "engine/custom_packet_manager.h"

#include <chrono>
#include <iphlpapi.h>
#include <util/utils.h>

using namespace std::literals::chrono_literals;

constexpr char LAN_BROADCAST_SCAN_MSG[] = "\xFF\xFF\xFF\xFF"
										  "\x4C" // This one is not taken afaik
										  "scan";

Lan* g_pLan;

Lan::Lan()
	: m_bIsBroadcastSocketOK(false)
	, m_broadcastEndpointSize(0)
	, m_broadcastSocket(0)
	, m_broadcastEndpoint({})
{
	static bool bInitialised = false;
	if (bInitialised)
		return;

	const auto wVersionRequested = MAKEWORD(2, 2);
	WSADATA wsaData;
	int lastError = WSAStartup(wVersionRequested, &wsaData);
	assert(!lastError);

	const bool isLanAvailable = lastError == ERROR_SUCCESS;
	SetupBroadcastSocket();

	bInitialised = true;
	m_bIsLanAvailable = isLanAvailable;
}

std::vector<ServerPresence> Lan::ScanForServers()
{
	std::vector<ServerPresence> presences {};

	// Independent thread is necessary for WSA error to function properly and not pollute the basegame
	std::thread t(
		[&]()
		{
			SendScanPing();
			presences = ReceivePresences();
		});

	t.join();

	return presences;
}

void Lan::DiscoverClient(const netadr_t& adr)
{

	std::lock_guard _(m_clientListMutex);
	LanServerReporter::ScanningClient client {};
	client.address = adr;

	m_clientsToReplyTo.emplace_back(client);
}

uint16_t Lan::GetBroadcastPort() const
{
	if (g_pCVar)
	{
		if (auto cvar = g_pCVar->FindVar("serverport"))
		{
			return static_cast<uint16_t>(cvar->GetInt());
		}
	}

	return 37015;
}

void Lan::SendScanPing()
{
	if (this->m_bIsBroadcastSocketOK)
	{
		int lastError = {};

		// Value used in NET_SendTo
		netPayload_s payload {};

		// We need to encrypt it first
		{
			sockaddr_in sin {};
			int addressLength = sizeof(sin);
			netadr_t r2address {};
			if (getsockname(m_broadcastSocket, reinterpret_cast<sockaddr*>(&sin), &addressLength) == 0 && sin.sin_family == AF_INET &&
				addressLength == sizeof(sin))
			{
				// We're gonna use this crypto provider only once so it does not really matter
				//   what IP we supply NET_Encrypt, let's give it the correct broadcast IP+Port still
				r2address.type = NA_IP;
				r2address.port = sin.sin_port;
				r2address.ip[10] = 0xFF; // IPV4 marker
				r2address.ip[11] = 0xFF;
				memcpy_s(&r2address.ip[12], 4, &this->m_broadcastEndpoint.Ipv4.sin_addr, sizeof(this->m_broadcastEndpoint.Ipv4.sin_addr));
			}
			else
			{
				assert(!(lastError = WSAGetLastError()));

				// No point in sending it because it won't be decryptable :(
				NS::log::NORTHSTAR->error("Failed to get local netadr_t for encryption!");
				return;
			}

			ZeroMemory(&payload, sizeof(payload));

			static_assert(sizeof(payload.encryptHeader.nonce) == 12);
			static_assert(sizeof(payload.encryptHeader.tag) == 16);

			// This is how the game does it
			int encryptedLength = NET_Encrypt(
				&r2address,
				LAN_BROADCAST_SCAN_MSG,
				sizeof(LAN_BROADCAST_SCAN_MSG),
				payload.messageData,
				sizeof(payload.messageData),
				&payload.encryptHeader.tag,
				sizeof(payload.encryptHeader.tag),
				&payload.encryptHeader.nonce,
				sizeof(payload.encryptHeader.nonce));

			assert(encryptedLength > 0);
			if (encryptedLength <= 0)
			{
				NS::log::NORTHSTAR->error("Failed to encrypt client scan message!");
				return;
			}

			encryptedLength += sizeof(netPayload_s::encryptHeader);

			const auto result = sendto(
				this->m_broadcastSocket,
				reinterpret_cast<const char*>(&payload),
				encryptedLength,
				0,
				reinterpret_cast<const sockaddr*>(&this->m_broadcastEndpoint),
				this->m_broadcastEndpointSize);

			switch (lastError = WSAGetLastError())
			{
				// We don't mind if the scan message bounces
			case WSAECONNREFUSED:
			case WSAEALREADY:
			case WSAECONNABORTED:
			case WSAETIMEDOUT:
				lastError = ERROR_SUCCESS;
				break;
			}

			assert(!lastError);
			this->m_bIsBroadcastSocketOK &= lastError == ERROR_SUCCESS;
		}
	}
}

std::vector<ServerPresence> Lan::ReceivePresences(uint32_t timeout)
{
	std::unordered_map<std::string, ServerPresence> discoveries {};

	if (this->m_bIsBroadcastSocketOK)
	{
		int lastError = {};

		setsockopt(this->m_broadcastSocket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
		assert(!(lastError = WSAGetLastError()));

		// expected response
		Lan::LanServerReporter::Payload presenceMessage {};

		_SOCKADDR_INET from {};
		int32_t fromLength = sizeof(from);

		while (true)
		{
			const auto received = recvfrom(
				this->m_broadcastSocket,
				reinterpret_cast<char*>(&presenceMessage),
				sizeof(presenceMessage),
				0,
				reinterpret_cast<sockaddr*>(&from),
				&fromLength);

			lastError = WSAGetLastError();
			if (lastError == WSAETIMEDOUT)
			{
				lastError = 0;
				break;
			}
			else if (lastError)
			{
				assert(!lastError);
				break;
			}
			else if (received == 0)
			{
				break; // Empty read is not supposed to happen in UDP but...
			}

			// We only allow exact size and we ignore fragmentation, since we're under the MTU and not in a Stream protocol
			// it should be fine
			if (received == sizeof(presenceMessage))
			{
				// A server can be reported twice if we share more than one network together
				const auto presence = presenceMessage.ToPresence(&from, fromLength);
				if (!discoveries.contains(presence.m_sServerId))
				{
					discoveries.insert(std::make_pair(presence.m_sServerId, presence));
				}
			}
		}

		this->m_bIsBroadcastSocketOK = lastError == ERROR_SUCCESS;
	}

	std::vector<ServerPresence> servers {};
	for (auto const& kv : discoveries)
	{
		servers.emplace_back(kv.second);
	}

	return servers;
}

void Lan::SetupBroadcastSocket()
{
	int lastError = {};

	this->m_bIsBroadcastSocketOK = false;

	this->m_broadcastSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	assert(!(lastError = WSAGetLastError()));

	sockaddr_in bEndpoint {};
	{
		const auto ip = INADDR_BROADCAST;
		bEndpoint.sin_family = AF_INET;
		memcpy_s(&bEndpoint.sin_addr, sizeof(bEndpoint.sin_addr), &ip, sizeof(ip));
		bEndpoint.sin_port = htons(GetBroadcastPort());
	}

	// No matter how big it is, this struct is big enough for it (whether ipv6 or ipv4)
	ZeroMemory(&this->m_broadcastEndpoint, sizeof(this->m_broadcastEndpoint));
	this->m_broadcastEndpoint = static_cast<SOCKADDR_INET>(bEndpoint);
	this->m_broadcastEndpointSize = sizeof(bEndpoint);

	// "Don't linger" might not be strictly necessary here but I usually add it to make sure the EXE won't linger in memory once the program
	// terminated, waiting for a connection to close
	constexpr bool dontLinger = true;
	constexpr bool broadcast = true;

	setsockopt(this->m_broadcastSocket, SOL_SOCKET, SO_DONTLINGER, reinterpret_cast<const char*>(&dontLinger), sizeof(dontLinger));
	setsockopt(this->m_broadcastSocket, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&broadcast), sizeof(broadcast));

	assert(!(lastError = WSAGetLastError()));

	sockaddr_in listenEndpoint {};
	listenEndpoint.sin_family = AF_INET;
	// Any port is good, we'll have to punch through the firewall anyway

	bind(m_broadcastSocket, reinterpret_cast<sockaddr*>(&listenEndpoint), sizeof(listenEndpoint));

	assert(!(lastError = WSAGetLastError()));

	this->m_bIsBroadcastSocketOK = lastError == ERROR_SUCCESS;
}

void Lan::LanServerReporter::ReportPresence(double flCurrentTime, const ServerPresence* pServerPresence)
{
	// Send unicast to broadcasting clients - default Windows firewall allows a single unicast response to a broadcast within 3
	// seconds of the emitted broadcast, so broadcasting has to be a client's job, not a server's job
	std::lock_guard _(g_pLan->m_clientListMutex);

	if (g_pLan->m_clientsToReplyTo.size() > 0)
	{
		// We don't trigger the presence cooldown if we did nothing - this allows the LAN server to answer broadcasts with minimal latency
		ServerPresenceReporter::ReportPresence(flCurrentTime, pServerPresence);
	}
	else
	{
		return; // Nothing to do here
	}

	for (const auto& client : g_pLan->m_clientsToReplyTo)
	{
		NS::log::NORTHSTAR->info("Replying with presence to client {}", client.ToString());

		Lan::LanServerReporter::Payload serializedPresence(pServerPresence);

		std::string data(reinterpret_cast<const char*>(&serializedPresence), sizeof(serializedPresence));

		netadr_s destination = client.address;

		constexpr auto SERVER_SOCKET_INDEX = 1;
		constexpr auto COMPRESSED = false;

		NET_SendPacket(nullptr, SERVER_SOCKET_INDEX, &destination, data.data(), data.size(), 0, COMPRESSED, 0, 0);
	}

	g_pLan->m_clientsToReplyTo.clear();
}

Lan::LanServerReporter::Payload::Payload(const ServerPresence* pServerPresence)
{
	// Copy simple properties
#define SET(x) this->x = pServerPresence->x
	SET(m_iPort);
	SET(m_bIsSingleplayerServer);

	SET(m_iPlayerCount);
	SET(m_iMaxPlayers);

#undef SET

	// Copy char buffers
#define SET(x)                                                                                                                             \
	assert(sizeof(pServerPresence->x) == sizeof(this->x));                                                                                 \
	std::memcpy(this->x, pServerPresence->x, sizeof(this->x))

	SET(m_Password);
	SET(m_MapName);
	SET(m_PlaylistName);

#undef SET

	// These two are strings!
#define SET_STRING(x)                                                                                                                      \
	ZeroMemory(this->x, sizeof(this->x));                                                                                                  \
	std::strncpy(this->x, pServerPresence->x.c_str(), sizeof(this->x));

	SET_STRING(m_sServerName);
	SET_STRING(m_sServerDesc);

#undef SET_STRING
}

ServerPresence Lan::LanServerReporter::Payload::ToPresence(const SOCKADDR_INET* from, int32_t fromLength)
{
	ServerPresence presence {};

	// Copy simple properties
#define SET(x) presence.x = this->x
	SET(m_iPort);
	SET(m_sServerName);
	SET(m_sServerDesc);
	SET(m_bIsSingleplayerServer);

	SET(m_iPlayerCount);
	SET(m_iMaxPlayers);

#undef SET

	// Copy char buffers
#define SET(x)                                                                                                                             \
	assert(sizeof(presence.x) == sizeof(this->x));                                                                                         \
	std::memcpy(presence.x, this->x, sizeof(this->x))

	SET(m_Password);
	SET(m_MapName);
	SET(m_PlaylistName);

#undef SET

	// Generate server ID from IP + port
	{
		char name[NI_MAXHOST] {};
		int lastError = getnameinfo(reinterpret_cast<const sockaddr*>(from), fromLength, name, sizeof(name), NULL, 0, NI_NUMERICHOST);

		assert(!lastError);
		if (lastError == ERROR_SUCCESS)
		{
			presence.m_sServerId = std::format("{}:{}", name, m_iPort);
		}
		else
		{
			// As a fallback
			presence.m_sServerId = std::format("{}:{}", m_sServerName, m_iPort);
		}
	}

	return presence;
}

ON_DLL_LOAD_RELIESON("engine.dll", LanDiscoveryPacketHandler, CustomPacketManager, (CModule module))
{
	assert(g_pCustomPacketManager);
	static_assert(ARRAYSIZE(LAN_BROADCAST_SCAN_MSG) > 4);

	g_pCustomPacketManager->RegisterPacketHandler(
		LAN_BROADCAST_SCAN_MSG[4],
		[](void* handler, netpacket_s* packet, OUT bool& executeOriginalHandler)
		{
			// Received a reply to my broadcast, so we need to reply to this client
			g_pLan->DiscoverClient(packet->adr);
		});
}
