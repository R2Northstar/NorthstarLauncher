#include "pch.h"
#include "squirrel.h"

// asset function StringToAsset( string assetName )
ADD_SQUIRREL_FUNC(
	"asset",
	StringToAsset,
	"string assetName",
	"converts a given string to an asset",
	(ScriptContext::UI | ScriptContext::CLIENT | ScriptContext::SERVER))
{
	g_pSquirrel<context>->pushasset(sqvm, g_pSquirrel<context>->getstring(sqvm, 1), -1);
	return SQRESULT_NOTNULL;
}
