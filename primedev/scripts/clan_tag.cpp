#include "squirrel/squirrel.h"
#include "dedicated/dedicated.h"
#include "masterserver/masterserver.h"

ADD_SQFUNC("bool", NSSetLocalPlayerClanTag, "string clantag", "", ScriptContext::CLIENT | ScriptContext::UI)
{
	std::string clantag = g_pSquirrel[context]->getstring(sqvm, 1);
	bool result = g_pMasterServerManager->SetLocalPlayerClanTag(clantag);
	g_pSquirrel[context]->pushbool(sqvm, result);
	return SQRESULT_NOTNULL;
}
