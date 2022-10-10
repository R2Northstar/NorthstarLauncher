#pragma once

#include "pch.h"

#include <regex>
#include "base64.h"
#include "convar.h"

static std::string URIProtocolName = "northstar://";

enum InviteType
{
	SERVER = 0,
	PARTY = 1,
};

inline std::optional<InviteType> invitetype_from_string(std::string input) {
	if (input == "server")
	{
		return InviteType::SERVER;
	}
	if (input == "party")
	{
		return InviteType::PARTY;
	}
	return std::nullopt;
}

inline std::string string_from_invitetype(InviteType type) {
	switch (type)
	{
		case InviteType::SERVER:
			return "server";
		case InviteType::PARTY:
			return "party";
		default:
			return "";
	}
}

class Invite
{
  public:
	InviteType type;
	bool active = false;
	std::string id = "";
	std::string password = "";
	std::string as_local_request();
	std::string as_uri();
	void store();
};

std::optional<Invite> parseURI(std::string uriString);

extern Invite* storedInvite;
extern ConVar* Cvar_ns_dont_ask_install_urihandler;
