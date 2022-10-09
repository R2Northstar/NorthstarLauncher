#pragma once
#include "pch.h"
#include "httplib.h"
#include "invites.h"

class URIHandler
{
  private:
	httplib::Server m_URIServer;

  public:
	void StartServer();
	/* bool HandleAcceptedInvite();
	void InitialiseURIStuff(HMODULE baseAddress);

	bool parseURI(std::string uriString);*/

	bool m_bIsRunning = false;
};

extern URIHandler* g_pURIHandler;
