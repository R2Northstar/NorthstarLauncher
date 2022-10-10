#pragma once

#include "pch.h"

#include <regex>
#include "base64.h"

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

	int uriOffset = uriString.find(URIProtocolName);
	if (uriOffset != std::string::npos)
	{
		uriString = uriString.substr(uriOffset + URIProtocolName.length(), uriString.length() - uriOffset - 1); // -1 to remove a trailing slash -_-
	}
	if (uriString[uriString.length() - 1] == '/')
	{
		uriString = uriString.substr(0, uriString.length() - 1);
	}
	
	std::regex r("(\\w*)@(\\w*):?(.*)");

	std::smatch matches;

	auto found = std::regex_match(uriString, matches, r);
	if (matches.size() < 4)
	{
		return std::nullopt;
	}
	auto maybe_type = invitetype_from_string(matches[1]);
	if (!maybe_type)
	{
		spdlog::warn("Tried parsing invite with invalid type '{}'", matches[1].str());
		return std::nullopt;
	}
	invite.type = maybe_type.value();
	invite.id = matches[2].str();
	invite.password = matches[3].str() == "" ? "" : base64_decode(matches[3].str());

	return invite;
}

extern Invite* storedInvite;
