#pragma once
#include "pch.h"
#include "bansystem.h"
#include "serverauthentication.h"
#include "concommand.h"
#include "miscserverscript.h"
#include <filesystem>
#include <iostream>
#include "configurables.h"
#include "masterserver.h"
/*
const char* BanMessages[] =
{
	"You farded and shidded, appeal at discord.gg/Northstar",
	"Authentication error, please enable mat_global_lighting 1",
	"uh owh! we madwe an oopsie woopsie >.< we ar workin vewy hawd two fwix it uwu",
	"Error code: 80085. Please check https://laund.moe/ to solve your issue.",
	"Congratulations, you are our 10,000th visitor!"
	"The server is going through a rough patch of its life and we want to give it time alone so it could sort its feelings. Please be understanding.",
	"Somebody once told me, the world is gonna roll me.",
	"Eternal banana.",
	"Unfortunately this server will “give you up, and let you down”",
	"Stop",
	"Banana bread. . .",
	"You have been banned due to using malicious tools to gain an unfair advantage. Your ban will be lifted on the release date of Titanfall 3.",
	"Did BT-7274 really sacrificed himself for a Pilot like you?",
	"Advancement Unlocked: How Did We Get Here?",
	"Now this is a story all about how, your life got flipped, turned upside-down. And I like to take a minute, just sit right there, and I'll tell you how you became a cheating prince of Bel-Air.",
	"Nice cheats, unfortunately for you, I LIVE IN YOUR WALLS.",
	"No bitches?"
};
*/

const char* BANLIST_PATH_SUFFIX = "/banlist.txt";
const char* BANMESSAGE_PATH_SUFFIX = "/banmessages.txt";
const char* DEFAULT_BAN_MESSAGE = "Banned from server";

ServerBanSystem* g_ServerBanSystem;

bool ServerBanSystem::ParseLocalBanMessageFile() 
{
	// Read and load /R2Northstar/banmessages.txt into m_vBanMessages
	std::ifstream BanmessagesStream(GetNorthstarPrefix() + "/banmessages.txt");
	if (!BanmessagesStream.fail())
	{
		spdlog::info("banmessages.txt found!");
		std::string line;
		while (std::getline(BanmessagesStream, line))
		{
			spdlog::info("added {} to banmessages", line);
			m_vBanMessages.push_back(line);
		}
		return true;
	}
	else 
	{
		//spdlog::info("banmessages.txt is not found! make sure it's properly placed in /R2Northstar/banmessages.txt. ignore this if you do not plan to use custom banmessages");
		return false;
	}
}

void ServerBanSystem::ParseRemoteBanlistString(std::string banlisttring)
{
	std::stringstream banliststream(banlisttring + "\n");
	uint64_t uid;
	//load banned UIDs from file
	std::ifstream enabledModsStream(GetNorthstarPrefix() + "/banlist.txt");
	std::stringstream enabledModsStringStream;
	if (!enabledModsStream.fail())
	{
		std::string line;
		m_vBannedUids.clear();//clear currently running bannedUID vec
		while (std::getline(enabledModsStream, line))
			m_vBannedUids.push_back(strtoll(line.c_str(), nullptr, 10));

		while (banliststream >> uid)
		{
			//spdlog::info("{}has been inserted into m_vBannedUids! ",uid);
			InsertBanUID(uid);
		}
		g_MasterServerManager->LocalBanlistVersion = g_MasterServerManager->RemoteBanlistVersion;
		banliststream.clear();
		banliststream.seekg(0);
		enabledModsStream.close();

	}
	//Add Remote BannedUIDs from Masterserver, without overwrtiing actual banlist.txt


}

const char* ServerBanSystem::GetRandomBanMessage()
{
	//Returns random message from m_vBanMessages
	if (m_vBanMessages.size() != 0)
	{
		m_CurrentMessageIndex = rand() % m_vBanMessages.size();
		const char* output = m_vBanMessages[m_CurrentMessageIndex].c_str();
		//spdlog::info("Chosen ban message:{}",output);
		return output;
	}
	else 
	{
		return DEFAULT_BAN_MESSAGE; //Return default ban message if no custom ban message is present
	}
	
}

void ServerBanSystem::InsertBanUID(uint64_t uid) // ban the player without fucking with existing banlist objects
{
	//todo: switch m_vBannedUids to HashSet
	//see: https://en.cppreference.com/w/cpp/container/unordered_set
	auto findResult = std::find(m_vBannedUids.begin(), m_vBannedUids.end(), uid);
	if (findResult == m_vBannedUids.end()) 
	{
		//cannot find UID in m_vBannedUids, perform ban
		m_vBannedUids.push_back(uid);
		std::string UidString = std::to_string(uid);
		for (int i = 0; i < 32; i++)
		{
			void* player = GetPlayerByIndex(i);

			if (!strcmp((char*)player + 0xF500, UidString.c_str()))
			{
				//kicks the player with ban message
				CBaseClient__Disconnect(player, 1, GetRandomBanMessage());
				break;
			}
		}
	}
	else
	{
		spdlog::info("Bypassing Incoming Ban from masterserver:{}, Player is already banned!", uid);
	}
}

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

void BanPlayerCommand(const CCommand& args)
{
	if (args.ArgC() < 2)
		return;

	// assuming maxplayers 32
	for (int i = 0; i < 32; i++)
	{
		void* player = GetPlayerByIndex(i);

		if (!strcmp((char*)player + 0x16, args.Arg(1)) || !strcmp((char*)player + 0xF500, args.Arg(1)))
		{
			g_ServerBanSystem->BanUID(strtoll((char*)player + 0xF500, nullptr, 10));
			CBaseClient__Disconnect(player, 1, g_ServerBanSystem->GetRandomBanMessage());
			break;
		}
	}
}

void UnbanPlayerCommand(const CCommand& args)
{
	if (args.ArgC() < 2)
		return;

	// assumedly the player being unbanned here wasn't already connected, so don't need to iterate over players or anything
	g_ServerBanSystem->UnbanUID(strtoll(args.Arg(1), nullptr, 10));
}

void ClearBanlistCommand(const CCommand& args) { g_ServerBanSystem->ClearBanlist(); }

void InitialiseBanSystem(HMODULE baseAddress)
{
	g_ServerBanSystem = new ServerBanSystem;
	g_ServerBanSystem->OpenBanlist();
	g_ServerBanSystem->ParseLocalBanMessageFile();
	RegisterConCommand("ban", BanPlayerCommand, "bans a given player by uid or name", FCVAR_GAMEDLL);
	RegisterConCommand("unban", UnbanPlayerCommand, "unbans a given player by uid", FCVAR_NONE);
	RegisterConCommand("clearbanlist", ClearBanlistCommand, "clears all uids on the banlist", FCVAR_NONE);
}