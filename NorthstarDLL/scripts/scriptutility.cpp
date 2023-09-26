#include "squirrel/squirrel.h"
#include "client/r2client.h"
#include "engine/r2engine.h"
#include "ns_version.h"

// asset function StringToAsset( string assetName )
ADD_SQFUNC(
	"asset",
	StringToAsset,
	"string assetName",
	"converts a given string to an asset",
	ScriptContext::UI | ScriptContext::CLIENT | ScriptContext::SERVER)
{
	g_pSquirrel<context>->pushasset(sqvm, g_pSquirrel<context>->getstring(sqvm, 1), -1);
	return SQRESULT_NOTNULL;
}

// string function NSGetLocalPlayerUID()
ADD_SQFUNC(
	"string", NSGetLocalPlayerUID, "", "Returns the local player's uid.", ScriptContext::UI | ScriptContext::CLIENT | ScriptContext::SERVER)
{
	if (R2::g_pLocalPlayerUserID)
	{
		g_pSquirrel<context>->pushstring(sqvm, R2::g_pLocalPlayerUserID);
		return SQRESULT_NOTNULL;
	}

	return SQRESULT_NULL;
}

ADD_SQFUNC(
	"string",
	GetNorthstarVersion,
	"",
	"Returns the current version string of Northstar(.dll)",
	ScriptContext::UI | ScriptContext::CLIENT | ScriptContext::SERVER)
{
	constexpr int version[4] {NORTHSTAR_VERSION};
	std::string versionStr = std::format("{}.{}.{}", version[0], version[1], version[2]);

	// for dev builds, show the fourth version number and the +dev bit
	if (version[3])
		versionStr += std::format(".{}+dev", version[3]);

	g_pSquirrel<context>->pushstring(sqvm, versionStr.c_str());
	return SQRESULT_NOTNULL;
}
