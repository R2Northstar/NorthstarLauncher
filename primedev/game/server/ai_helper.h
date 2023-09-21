#pragma once

#include "game/server/ai_node.h"
#include "game/server/ai_navmesh.h"

dtNavMesh* GetNavMeshForHull(int nHull);

class CAI_Helper
{
  public:
	void SaveNetworkGraph(CAI_Network* pNetwork);
	void DrawNetwork(CAI_Network* pNetwork);
	void DrawNavmeshPolys(dtNavMesh* pNavMesh = nullptr);
};

inline CAI_Helper* g_pAIHelper = nullptr;
