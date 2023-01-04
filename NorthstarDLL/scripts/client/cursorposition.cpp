#include "pch.h"
#include "squirrel/squirrel.h"
#include "util/wininfo.h"

ADD_SQFUNC("vector ornull", NSGetCursorPosition, "", "", ScriptContext::UI)
{
	POINT p;
	if (GetCursorPos(&p) && ScreenToClient(*g_gameHWND, &p))
	{
		if (GetAncestor(GetForegroundWindow(), GA_ROOTOWNER) != *g_gameHWND)
			return SQRESULT_NULL;

		g_pSquirrel<context>->pushvector(sqvm, {(float)p.x, (float)p.y, 0});
		return SQRESULT_NOTNULL;
	}
	g_pSquirrel<context>->raiseerror(sqvm, "Failed retrieving cursor position of game window");
	return SQRESULT_ERROR;
}
