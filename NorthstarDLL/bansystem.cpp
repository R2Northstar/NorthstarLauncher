#pragma once
#include "pch.h"
#include "bansystem.h"
#include "serverauthentication.h"
#include "concommand.h"
#include "r2server.h"
#include "r2engine.h"
#include "nsprefix.h"

#include <filesystem>

const char* BANLIST_PATH_SUFFIX = "/banlist.txt";

ServerBanSystem* g_pBanSystem;

void ServerBanSystem::OpenBanlist()
{
	std::ifstream enabledModsStream(GetNorthstarPrefix() + "/banlist.txt");
	std::stringstream enabledModsStringStream;

	if (!enabledModsStream.fail())
	{
		std::string line;
		while (std::getline(enabledModsStream, line))
			m_vBannedUids.push_back(strtoll(line.c_str(), nullptr, 10));

		enabledModsStream.close();
	}

	// open write stream for banlist
	m_sBanlistStream.open(GetNorthstarPrefix() + "/banlist.txt", std::ofstream::out | std::ofstream::binary | std::ofstream::app);
}

void ServerBanSystem::ClearBanlist()
{
	m_vBannedUids.clear();

	// reopen the file, don't provide std::ofstream::app so it clears on open
	m_sBanlistStream.close();
	m_sBanlistStream.open(GetNorthstarPrefix() + "/banlist.txt", std::ofstream::out | std::ofstream::binary);
}

void ServerBanSystem::BanUID(uint64_t uid)
{
	m_vBannedUids.push_back(uid);
	m_sBanlistStream << std::to_string(uid) << std::endl;
	spdlog::info("{} was banned", uid);
}

void ServerBanSystem::UnbanUID(uint64_t uid)
{
	auto findResult = std::find(m_vBannedUids.begin(), m_vBannedUids.end(), uid);
	if (findResult == m_vBannedUids.end())
		return;

	m_vBannedUids.erase(findResult);
	spdlog::info("{} was unbanned", uid);
	// todo: this needs to erase from the banlist file
	// atm unsure how to do this aside from just clearing and fully rewriting the file
}

bool ServerBanSystem::IsUIDAllowed(uint64_t uid)
{
	return std::find(m_vBannedUids.begin(), m_vBannedUids.end(), uid) == m_vBannedUids.end();
}

void ConCommand_ban(const CCommand& args)
{
	if (args.ArgC() < 2)
		return;

	// assuming maxplayers 32
	for (int i = 0; i < 32; i++)
	{
		R2::CBaseClient* player = &R2::g_pClientArray[i];

		if (!strcmp(player->m_Name, args.Arg(1)) || !strcmp(player->m_UID, args.Arg(1)))
		{
			g_pBanSystem->BanUID(strtoll((char*)player + 0xF500, nullptr, 10));
			R2::CBaseClient__Disconnect(player, 1, "Banned from server");
			break;
		}
	}
}

void ConCommand_unban(const CCommand& args)
{
	if (args.ArgC() < 2)
		return;

	// assumedly the player being unbanned here wasn't already connected, so don't need to iterate over players or anything
	g_pBanSystem->UnbanUID(strtoll(args.Arg(1), nullptr, 10));
}

void ConCommand_clearbanlist(const CCommand& args)
{
	g_pBanSystem->ClearBanlist();
}

ON_DLL_LOAD_RELIESON("engine.dll", BanSystem, ConCommand, (CModule module))
{
	g_pBanSystem = new ServerBanSystem;
	g_pBanSystem->OpenBanlist();

	RegisterConCommand("ban", ConCommand_ban, "bans a given player by uid or name", FCVAR_GAMEDLL);
	RegisterConCommand("unban", ConCommand_unban, "unbans a given player by uid", FCVAR_GAMEDLL);
	RegisterConCommand("clearbanlist", ConCommand_clearbanlist, "clears all uids on the banlist", FCVAR_GAMEDLL);
}