#pragma once

static std::string URIProtocolName = "northstar://";

bool parseURI(std::string uriString);

void HandleAcceptedInvite();
void InitialiseURIStuff(HMODULE baseAddress);