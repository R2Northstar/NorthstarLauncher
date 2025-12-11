#include "squirrel/squirrel.h"
#include "../../imgui/imgui_ws_test.h"

ADD_SQFUNC("void", NSDrawImGuiFromScript, "string message", "", ScriptContext::UI)
{
	const SQChar* message = g_pSquirrel<context>->getstring(sqvm, 1);

	RenderScriptThing(message);

	return SQRESULT_NULL;
}
