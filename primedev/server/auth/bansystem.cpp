#include "bansystem.h"
#include "serverauthentication.h"
#include "core/convar/concommand.h"
#include "server/r2server.h"
#include "engine/r2engine.h"
#include "client/r2client.h"
#include "config/profile.h"
#include "shared/maxplayers.h"

#include <filesystem>
#include <stdio.h>
#include <string.h>

#include <algorithm>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cctype>
#include <iostream>

const char* BANLIST_PATH_SUFFIX = "/banlist.txt";
const char BANLIST_COMMENT_CHAR = '#';
const char BANLIST_REASON_SEPARATOR = '|';

ServerBanSystem* g_pBanSystem;

static inline std::string Trim(const std::string& s)
{
	size_t start = 0;
	while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
		++start;
	if (start == s.size())
		return "";

	size_t end = s.size() - 1;
	while (end > start && std::isspace(static_cast<unsigned char>(s[end])))
		--end;

	return s.substr(start, end - start + 1);
}

// Parse a line from banlist into (uidStr, reason) if it's a data line.
// Returns true if the line contained a valid-looking uid (digits) and fills uidStr/reason (reason may be empty).
// If the line is empty or a comment-only line returns false.
static bool ParseBanlistLine(const std::string& rawLine, std::string& outUidStr, std::string& outReason)
{
	// Trim leading/trailing whitespace for parsing purposes
	std::string line = Trim(rawLine);

	// empty line
	if (line.empty())
		return false;

	// comment line (first non-space char is comment)
	if (line.front() == BANLIST_COMMENT_CHAR)
		return false;

	// strip inline comment (everything after first '#')
	size_t commentPos = line.find(BANLIST_COMMENT_CHAR);
	std::string dataPart = (commentPos == std::string::npos) ? line : line.substr(0, commentPos);
	dataPart = Trim(dataPart);
	if (dataPart.empty())
		return false;

	// split by first '|' into uid and reason (reason may contain spaces; we trim only ends)
	size_t sep = dataPart.find(BANLIST_REASON_SEPARATOR);
	if (sep != std::string::npos)
	{
		outUidStr = Trim(dataPart.substr(0, sep));
		outReason = Trim(dataPart.substr(sep + 1));
	}
	else
	{
		outUidStr = Trim(dataPart);
		outReason.clear();
	}

	// uid should be digits only
	if (outUidStr.empty())
		return false;
	for (char c : outUidStr)
	{
		if (!std::isdigit(static_cast<unsigned char>(c)))
			return false;
	}

	return true;
}

void ServerBanSystem::OpenBanlist()
{
	const std::string path = GetNorthstarPrefix() + BANLIST_PATH_SUFFIX;
	std::ifstream banlistStream(path);

	if (!banlistStream.fail())
		return;

	std::string line;
	while (std::getline(banlistStream, line))
	{
		std::string uidStr;
		std::string reason;
		if (!ParseBanlistLine(line, uidStr, reason))
			continue;

		uint64_t uid = strtoull(uidStr.c_str(), nullptr, 10);
		m_vBannedPlayers.push_back({uid, reason});
	}

	banlistStream.close();
}

void ServerBanSystem::ReloadBanlist()
{
	const std::string path = GetNorthstarPrefix() + BANLIST_PATH_SUFFIX;
	std::ifstream fsBanlist(path);

	if (fsBanlist.fail())
		return;

	std::string line;
	m_vBannedPlayers.clear();
	while (std::getline(fsBanlist, line))
	{
		std::string uidStr;
		std::string reason;
		if (!ParseBanlistLine(line, uidStr, reason))
			continue;

		m_vBannedPlayers.push_back({strtoull(uidStr.c_str(), nullptr, 10), reason});
	}

	fsBanlist.close();
}

void ServerBanSystem::ClearBanlist()
{
	m_vBannedPlayers.clear();

	// reopen the file, don't provide std::ofstream::app so it clears on open
	if (m_sBanlistStream.is_open())
		m_sBanlistStream.close();

	m_sBanlistStream.open(GetNorthstarPrefix() + BANLIST_PATH_SUFFIX, std::ofstream::out | std::ofstream::binary);
	m_sBanlistStream.close();
}

void ServerBanSystem::BanUID(uint64_t uid, const std::string& reason)
{
	// Avoid double-adding the same UID
	auto exists = std::find_if(m_vBannedPlayers.begin(), m_vBannedPlayers.end(), [uid](const BannedPlayer& p) { return p.uid == uid; });
	if (exists != m_vBannedPlayers.end())
	{
		// If already present, optionally update reason (only if provided)
		if (!reason.empty())
			exists->reason = reason;
		// Still ensure file contains the (first) entry; for simplicity we append only if not present
		return;
	}

	// checking if last char is \n to make sure uids arent getting fucked
	const std::string path = GetNorthstarPrefix() + BANLIST_PATH_SUFFIX;
	std::ifstream fsBanlist(path);
	std::string content;
	if (!fsBanlist.fail())
		content.assign((std::istreambuf_iterator<char>(fsBanlist)), std::istreambuf_iterator<char>());
	fsBanlist.close();

	if (m_sBanlistStream.is_open())
		m_sBanlistStream.close();

	m_sBanlistStream.open(path, std::ofstream::out | std::ofstream::binary | std::ofstream::app);
	if (!m_sBanlistStream.is_open())
	{
		spdlog::error("Failed to open banlist for appending: {}", path);
		return;
	}

	if (!content.empty() && content.back() != '\n')
		m_sBanlistStream << std::endl;

	m_vBannedPlayers.push_back({uid, reason});

	// Only write separator if reason is not empty
	if (!reason.empty())
		m_sBanlistStream << std::to_string(uid) << BANLIST_REASON_SEPARATOR << reason << std::endl;
	else
		m_sBanlistStream << std::to_string(uid) << std::endl;

	m_sBanlistStream.close();

	if (!reason.empty())
		spdlog::info("{} was banned for: {}", uid, reason);
	else
		spdlog::info("{} was banned", uid);
}

void ServerBanSystem::UnbanUID(uint64_t uid)
{
	auto findResult =
		std::find_if(m_vBannedPlayers.begin(), m_vBannedPlayers.end(), [uid](const BannedPlayer& player) { return player.uid == uid; });

	if (findResult == m_vBannedPlayers.end())
	{
		// still attempt to modify file in case memory and file were out of sync
	}

	// We'll read the whole banlist file, comment out the matching uid lines (preserving the original line as-is),
	// and add an unban timestamp.
	std::vector<std::string> banlistText;
	const std::string path = GetNorthstarPrefix() + BANLIST_PATH_SUFFIX;
	std::ifstream fs_readBanlist(path);

	if (!fs_readBanlist.fail())
	{
		std::string line;
		while (std::getline(fs_readBanlist, line))
		{
			// Parse the line to extract uid (if any)
			std::string uidStr;
			std::string reason;
			if (!ParseBanlistLine(line, uidStr, reason))
			{
				// comment/empty/invalid line: keep as-is
				banlistText.push_back(line);
				continue;
			}

			// If uid matches, comment the line and add unban timestamp
			if (uidStr == std::to_string(uid))
			{
				std::string commentedLine = std::string("# ") + line;

				// add a comment with unban date
				std::time_t t = std::time(nullptr);
				std::tm nowTm;
#if defined(_WIN32) || defined(_WIN64)
				localtime_s(&nowTm, &t);
#else
				localtime_r(&t, &nowTm);
#endif

				std::ostringstream unbanComment;
				// format: 2026-01-10 13:45
				unbanComment << " # unban date: ";
				unbanComment << (nowTm.tm_year + 1900) << "-";
				unbanComment << std::setw(2) << std::setfill('0') << (nowTm.tm_mon + 1) << "-";
				unbanComment << std::setw(2) << std::setfill('0') << nowTm.tm_mday << " ";
				unbanComment << std::setw(2) << std::setfill('0') << nowTm.tm_hour << ":";
				unbanComment << std::setw(2) << std::setfill('0') << nowTm.tm_min;

				commentedLine.append(unbanComment.str());
				banlistText.push_back(commentedLine);

				// remove from in-memory list if present
				if (findResult != m_vBannedPlayers.end())
					m_vBannedPlayers.erase(findResult);
			}
			else
			{
				banlistText.push_back(line);
			}
		}

		fs_readBanlist.close();
	}

	// open write stream for banlist // without append so we clear the file
	if (m_sBanlistStream.is_open())
		m_sBanlistStream.close();

	m_sBanlistStream.open(path, std::ofstream::out | std::ofstream::binary);
	if (!m_sBanlistStream.is_open())
	{
		spdlog::error("Failed to open banlist for writing: {}", path);
		return;
	}

	for (const std::string& updatedLine : banlistText)
		m_sBanlistStream << updatedLine << std::endl;

	m_sBanlistStream.close();
	spdlog::info("{} was unbanned", uid);
}

bool ServerBanSystem::IsUIDAllowed(uint64_t uid)
{
	uint64_t localPlayerUserID = strtoull(g_pLocalPlayerUserID, nullptr, 10);
	if (localPlayerUserID == uid)
		return true;

	ReloadBanlist();
	auto it =
		std::find_if(m_vBannedPlayers.begin(), m_vBannedPlayers.end(), [uid](const BannedPlayer& player) { return player.uid == uid; });
	return it == m_vBannedPlayers.end();
}

std::string ServerBanSystem::GetBanReason(uint64_t uid)
{
	auto it =
		std::find_if(m_vBannedPlayers.begin(), m_vBannedPlayers.end(), [uid](const BannedPlayer& player) { return player.uid == uid; });

	return it != m_vBannedPlayers.end() ? it->reason : "";
}

void ConCommand_ban(const CCommand& args)
{
	if (args.ArgC() < 2)
		return;

	std::string reason = "";
	if (args.ArgC() >= 3)
		reason = args.Arg(2);

	for (int i = 0; i < g_pGlobals->m_nMaxClients; i++)
	{
		CBaseClient* player = &g_pClientArray[i];

		if (!strcmp(player->m_Name, args.Arg(1)) || !strcmp(player->m_UID, args.Arg(1)))
		{
			g_pBanSystem->BanUID(strtoull(player->m_UID, nullptr, 10), reason);

			std::string disconnectMsg = "Banned from server";
			if (!reason.empty())
				disconnectMsg += ": " + reason;

			CBaseClient__Disconnect(player, 1, disconnectMsg.c_str());
			break;
		}
	}
}

void ConCommand_unban(const CCommand& args)
{
	if (args.ArgC() < 2)
		return;

	// assumedly the player being unbanned here wasn't already connected, so don't need to iterate over players or anything
	g_pBanSystem->UnbanUID(strtoull(args.Arg(1), nullptr, 10));
}

void ConCommand_clearbanlist(const CCommand& args)
{
	NOTE_UNUSED(args);
	g_pBanSystem->ClearBanlist();
	spdlog::info("Banlist cleared");
}

int ConCommand_banCompletion(const char* const partial, char commands[COMMAND_COMPLETION_MAXITEMS][COMMAND_COMPLETION_ITEM_LENGTH])
{
	const char* space = strchr(partial, ' ');
	const char* cmdName = partial;
	const char* query = partial + (space == nullptr ? 0 : (space - partial) + 1);

	const size_t queryLength = strlen(query);
	const size_t cmdLength = (space == nullptr) ? strlen(partial) : (space - partial + 1); // include the space if present

	int numCompletions = 0;
	for (int i = 0; i < GetMaxPlayers() && numCompletions < COMMAND_COMPLETION_MAXITEMS - 2; i++)
	{
		CBaseClient* client = &g_pClientArray[i];
		if (client->m_Signon < eSignonState::CONNECTED)
			continue;

		if (queryLength == 0 || !strncmp(query, client->m_Name, queryLength))
		{
			// copy prefix ("ban " or whole partial if no space)
			strncpy(commands[numCompletions], cmdName, cmdLength);
			// safely append the name and ensure null termination
			strncpy_s(
				commands[numCompletions] + cmdLength,
				COMMAND_COMPLETION_ITEM_LENGTH - cmdLength,
				client->m_Name,
				COMMAND_COMPLETION_ITEM_LENGTH - cmdLength - 1);
			commands[numCompletions][COMMAND_COMPLETION_ITEM_LENGTH - 1] = '\0';
			numCompletions++;
			if (numCompletions >= COMMAND_COMPLETION_MAXITEMS - 2)
				break;
		}

		if (queryLength == 0 || !strncmp(query, client->m_UID, queryLength))
		{
			strncpy(commands[numCompletions], cmdName, cmdLength);
			strncpy_s(
				commands[numCompletions] + cmdLength,
				COMMAND_COMPLETION_ITEM_LENGTH - cmdLength,
				client->m_UID,
				COMMAND_COMPLETION_ITEM_LENGTH - cmdLength - 1);
			commands[numCompletions][COMMAND_COMPLETION_ITEM_LENGTH - 1] = '\0';
			numCompletions++;
			if (numCompletions >= COMMAND_COMPLETION_MAXITEMS - 2)
				break;
		}
	}

	return numCompletions;
}

ON_DLL_LOAD_RELIESON("engine.dll", BanSystem, ConCommand, (CModule module))
{
	g_pBanSystem = new ServerBanSystem;
	g_pBanSystem->OpenBanlist();

	RegisterConCommand("ban", ConCommand_ban, "bans a given player by uid or name", FCVAR_GAMEDLL, ConCommand_banCompletion);
	RegisterConCommand("unban", ConCommand_unban, "unbans a given player by uid", FCVAR_GAMEDLL);
	RegisterConCommand("clearbanlist", ConCommand_clearbanlist, "clears all uids on the banlist", FCVAR_GAMEDLL);
}

ON_DLL_LOAD_RELIESON("server.dll", KickCompletion, ConCommand, (CModule module))
{
	ConCommand* kick = g_pCVar->FindCommand("kick");
	kick->m_pCompletionCallback = ConCommand_banCompletion;
	kick->m_nCallbackFlags |= 0x3;
}
