#pragma once
#include <fstream>
#include <vector>

struct BannedPlayer
{
	uint64_t uid;
	std::string reason;
};

class ServerBanSystem
{
private:
	std::ofstream m_sBanlistStream;
	std::vector<BannedPlayer> m_vBannedPlayers;

public:
	void OpenBanlist();
	void ReloadBanlist();
	void ClearBanlist();
	void BanUID(uint64_t uid, const std::string& reason = "");
	void UnbanUID(uint64_t uid);
	bool IsUIDAllowed(uint64_t uid);
	std::string GetBanReason(uint64_t uid);
};

extern ServerBanSystem* g_pBanSystem;
