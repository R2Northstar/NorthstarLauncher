#include "ai_navmesh.h"
#include "core/hooks.h"

ON_DLL_LOAD("server.dll", ServerAiNavMesh, (CModule module))
{
	g_pNavMesh = module.Offset(0x105F5D0).RCast<dtNavMesh**>();
}
