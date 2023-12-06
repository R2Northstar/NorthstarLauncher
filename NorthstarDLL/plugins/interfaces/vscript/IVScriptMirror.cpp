#include "IVScriptMirror.h"

class VScriptClientMirror : IVScriptMirror
{
	public:
	SQBool sq_getbool(HSquirrelVM* sqvm, SQInteger stackPos)
	{
		return g_pSquirrel<ScriptContext::CLIENT>->sq_getbool(sqvm, stackPos);
	}

	void sq_pushbool(HSquirrelVM* sqvm, SQBool b)
	{
		return g_pSquirrel<ScriptContext::CLIENT>->sq_pushbool(sqvm, stackPos);
	}
};

EXPOSE_SINGLE_INTERFACE(VScriptClientMirror, IVScriptMirror, VSCRIPT_CLIENT_VERSION);
