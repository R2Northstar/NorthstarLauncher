#pragma once
#include <fstream>

class ServerBanSystem
{
  private:
	
	std::ofstream m_sBanlistStream;
	std::vector<uint64_t> m_vBannedUids;
	std::vector<std::string> m_vBanMessages;
	int m_CurrentMessageIndex;

  public:
	const char* GetRandomBanMessage();
	void OpenBanlist();
	void ClearBanlist();
	void BanUID(uint64_t uid);
	void UnbanUID(uint64_t uid);
	bool IsUIDAllowed(uint64_t uid);
	void InsertBanUID(uint64_t uid);
	void ParseRemoteBanlistString(std::string banlisttring);
	bool ParseLocalBanMessageFile();
};

extern ServerBanSystem* g_ServerBanSystem;

void InitialiseBanSystem(HMODULE baseAddress);