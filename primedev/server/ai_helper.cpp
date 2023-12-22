#include "ai_helper.h"

#include "engine/hoststate.h"

#include "core/math/vplane.h"

#include <fstream>

const int AINET_VERSION_NUMBER = 57;
const int AINET_SCRIPT_VERSION_NUMBER = 21;
const int PLACEHOLDER_CRC = 0;

static ConVar* Cvar_navmesh_debug_hull;
static ConVar* Cvar_navmesh_debug_camera_radius;
static ConVar* Cvar_navmesh_debug_draw_type;

//-----------------------------------------------------------------------------
// Purpose: Get navmesh pointer for hull
// Output : navmesh*, nullptr if out of range
//-----------------------------------------------------------------------------
dtNavMesh* GetNavMeshForHull(int nHull)
{
	if (nHull < 1 || nHull > 4)
		return nullptr;

	return g_pNavMesh[nHull - 1];
}

//-----------------------------------------------------------------------------
// Purpose: Draw navmesh polys using debug overlay
// Input  : *pNavMesh
//-----------------------------------------------------------------------------
void CAI_Helper::DrawNavmeshPolys(dtNavMesh* pNavMesh)
{
	if (!pNavMesh)
		pNavMesh = GetNavMeshForHull(Cvar_navmesh_debug_hull->GetInt());
	if (!pNavMesh)
		return;

	Vector3 vCamera;
	QAngle aCamera;
	float fFov;
	g_pClientTools->GetLocalPlayerEyePosition(vCamera, aCamera, fFov);

	VPlane CullPlane(vCamera - aCamera.GetNormal() * 256.0f, aCamera);

	const float fCamRadius = Cvar_navmesh_debug_camera_radius->GetFloat();
	const int iDrawType = Cvar_navmesh_debug_draw_type->GetInt();

	for (int i = 0; i < pNavMesh->m_maxTiles; ++i)
	{
		const dtMeshTile* pTile = &pNavMesh->m_tiles[i];

		if (!pTile->header)
			continue;

		for (int j = 0; j < pTile->header->polyCount; j++)
		{
			const dtPoly* pPoly = &pTile->polys[j];

			if (vCamera.DistTo(pPoly->org) > fCamRadius)
				continue;

			if (CullPlane.GetPointSide(pPoly->org) != SIDE_FRONT)
				continue;

			const unsigned int ip = (unsigned int)(pPoly - pTile->polys);

			if (pPoly->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
			{
				const dtOffMeshConnection* pCon = &pTile->offMeshConnections[ip - pTile->header->offMeshBase];
				RenderLine(pCon->origin, pCon->dest, Color(255, 250, 50, 255), true);
			}
			else
			{
				const dtPolyDetail* pDetail = &pTile->detailMeshes[ip];

				Vector3 v[3];

				for (int k = 0; k < pDetail->triCount; ++k)
				{
					const unsigned char* t = &pTile->detailTris[(pDetail->triBase + k) * 4];
					for (int l = 0; l < 3; ++l)
					{
						if (t[l] < pPoly->vertCount)
						{
							float* pfVerts = &pTile->verts[pPoly->verts[t[l]] * 3];
							v[l] = Vector3(pfVerts[0], pfVerts[1], pfVerts[2]);
						}
						else
						{
							float* pfVerts = &pTile->detailVerts[(pDetail->vertBase + t[l] - pPoly->vertCount) * 3];
							v[l] = Vector3(pfVerts[0], pfVerts[1], pfVerts[2]);
						}
					}

					if (!(iDrawType & 2))
					{
						RenderTriangle(v[0], v[1], v[2], Color(110, 200, 220, 160), true);
					}

					if (!(iDrawType & 1))
					{
						RenderLine(v[0], v[1], Color(0, 0, 150), true);
						RenderLine(v[1], v[2], Color(0, 0, 150), true);
						RenderLine(v[2], v[0], Color(0, 0, 150), true);
					}
				}
			}
		}
	}
}

ON_DLL_LOAD("server.dll", ServerAIHelper, (CModule module))
{
	Cvar_navmesh_debug_hull = new ConVar("navmesh_debug_hull", "0", FCVAR_RELEASE, "0 = NONE");
	Cvar_navmesh_debug_camera_radius =
		new ConVar("navmesh_debug_camera_radius", "1000", FCVAR_RELEASE, "Radius in which to draw navmeshes");
	Cvar_navmesh_debug_draw_type =
		new ConVar("navmesh_debug_draw_type", "0", FCVAR_RELEASE, "0 = fill + wireframe, 1 = fill, 2 = wireframe");
}
