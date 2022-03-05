#pragma once

static std::string URIProtocolName = "northstar://";

bool parseURI(std::string uriString);

bool HandleAcceptedInvite();
void InitialiseURIStuff(HMODULE baseAddress);