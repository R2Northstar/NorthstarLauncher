#pragma once

static std::string URIProtocolName = "northstar://"; 

void parseURI(std::string uriString);

void HandleAcceptedInvite();
void InitialiseURIStuff(HMODULE baseAddress);