#pragma once

#include "pch.h"

static std::string URIProtocolName = "northstar://";

enum InviteType
{
	SERVER = 0,
	PARTY = 1
};

class Invite
{
  public:
	InviteType type;
	bool active = false;
	std::string id = "";
	std::string password = "";
	std::string as_url();
	void store();
};

inline std::optional<Invite> parseURI(std::string uriString)
{
	Invite invite = {};

	std::string password;
	std::string inviteType;
	int atLocation;
	int uriOffset = URIProtocolName.length();
	if (uriString.find(URIProtocolName) != std::string::npos)
	{
		if (strncmp(uriString.c_str() + (uriString.length() - 1), "/", 1))
		{
			uriString = uriString.substr(uriOffset, uriString.length() - uriOffset);
		}
	}
	int inviteOffset = uriString.find("/invite/");
	if (uriString.find("/invite/") != std::string::npos)
	{
		uriString = uriString.substr(inviteOffset + 8, uriString.length() - inviteOffset);
		atLocation = uriString.find("/");
	}
	else
	{
		atLocation = uriString.find("@");
	}

	spdlog::info("Parsing URI: {}", uriString.c_str());

	if (atLocation == std::string::npos)
	{
		// spdlog::info("Invalid or malformed URI. Returning early.");
		return std::nullopt;
	}
	std::string invitetype = uriString.substr(0, atLocation);
	if (invitetype == "server")
	{
		invite.type = InviteType::SERVER;
	}
	else if (invitetype == "party")
	{
		invite.type = InviteType::PARTY;
	}
	else
	{
		//spdlog::info("Invalid or unknown invite type \"{}\". Returning early.", invitetype);
		return std::nullopt;
	}
	uriString = uriString.substr(atLocation + 1);
	int sep = uriString.find(":");
	if (sep != std::string::npos)
	{
		invite.id = uriString.substr(0, sep);
		invite.password = uriString.substr(sep + 1); // This crashes when the input b64 is invalid, whatever
	}
	else
	{
		invite.id = uriString;
		invite.password = "";
	}

	return invite;
}

extern Invite* storedInvite;
