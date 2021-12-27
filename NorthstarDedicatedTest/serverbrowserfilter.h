#pragma once
#include <regex>
#include <vector>

class RemoteServerInfo;

class ServerBrowserFilter
{
private:
	bool m_enable = false;
	std::string m_rawPattern;
	std::regex m_pattern;
	std::vector<RemoteServerInfo> m_servers;
public:
	void SetPattern(char* pattern);
	void UpdateList();

	int GetServerCount();
	RemoteServerInfo GetServer(int index);
	std::string GetRawPattern();
};

