#include "pch.h"
#include "squirrel.h"
#include "wininfo.h"

ADD_SQFUNC("array<int>", NSGetCursorPosition, "", "", ScriptContext::UI)
{
	POINT p;
	if (GetCursorPos(&p) && ScreenToClient(*g_gameHWND, &p))
	{
		g_pSquirrel<context>->newarray(sqvm, 0);
		g_pSquirrel<context>->pushinteger(sqvm, p.x);
		g_pSquirrel<context>->arrayappend(sqvm, -2);
		g_pSquirrel<context>->pushinteger(sqvm, p.y);
		g_pSquirrel<context>->arrayappend(sqvm, -2);
		return SQRESULT_NOTNULL;
	}
	g_pSquirrel<context>->raiseerror(
		sqvm, "Failed retrieving cursor position of game window");
	return SQRESULT_ERROR;
}
