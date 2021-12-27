#include "pch.h"
#include "serverbrowserfilter.h"
#include "masterserver.h"

void ServerBrowserFilter::SetPattern(char* pattern)
{
	m_rawPattern = "";
	m_enable = false;

	// check we have a pattern
	if ((pattern == NULL) || (pattern[0] == '\0')) {
		spdlog::info("Disable server filter (pattern empty)");
	}
	else {
		m_rawPattern = std::string(pattern);
		// don't crash due to invalid syntax
		try {
			std::regex re = std::regex::basic_regex(pattern);
			m_pattern = re;
			m_enable = true;
			spdlog::info("Set server filter to \"{}\"", pattern);
		}
		catch (std::regex_error e) {
			// compiling pattern failed
			m_enable = false;
			spdlog::info("Invalid search pattern \"{}\"", pattern);
		}
	}

	UpdateList();
}

void ServerBrowserFilter::UpdateList()
{
	// clear current list
	m_servers.clear();

	// filter servers by name
	std::copy_if(
		g_MasterServerManager->m_remoteServers.begin(),
		g_MasterServerManager->m_remoteServers.end(),
		std::back_inserter(m_servers),
		[this](RemoteServerInfo serverInfo) {
			std::string serverName(serverInfo.name);
			bool match = !m_enable || std::regex_search(serverName, m_pattern);
			// spdlog::info("Server \"{}\" (id=\"{}\") matched query: {}", serverName, serverInfo.id, match);
			return match;
		}
	);
	// spdlog::info("Servers after filtering: {}", m_servers.size());
}


int ServerBrowserFilter::GetServerCount()
{
	return m_servers.size();
}

RemoteServerInfo& ServerBrowserFilter::GetServer(int index)
{
	return m_servers[index];
}

std::string ServerBrowserFilter::GetRawPattern()
{
	return m_rawPattern;
}