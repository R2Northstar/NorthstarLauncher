#include "game/server/ai_helper.h"

#include "game/client/cdll_client_int.h"
#include "engine/hoststate.h"
#include "engine/host.h"
#include "engine/edict.h"
#include "engine/debugoverlay.h"

#include "mathlib/vplane.h"

#include <fstream>

const int AINET_VERSION_NUMBER = 57;
const int AINET_SCRIPT_VERSION_NUMBER = 21;
const int PLACEHOLDER_CRC = 0;

int* pUnkStruct0Count;
UnkNodeStruct0*** pppUnkNodeStruct0s;
int* pUnkLinkStruct1Count;
UnkLinkStruct1*** pppUnkStruct1s;

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
// Purpose: Save network graph to disk
// Input  : *aiNetwork
//-----------------------------------------------------------------------------
void CAI_Helper::SaveNetworkGraph(CAI_Network* aiNetwork)
{
	// FIXME [Fifty]: Use new classes for this, clean up
	fs::path writePath(fmt::format("{}/maps/graphs", g_pEngineParms->szModName));
	writePath /= g_pServerGlobalVariables->m_pMapName;
	writePath += ".ain";

	// dump from memory
	DevMsg(eLog::NS, "writing ain file %s\n", writePath.string().c_str());

	std::ofstream writeStream(writePath, std::ofstream::binary);
	DevMsg(eLog::NS, "writing ainet version: %i\n", AINET_VERSION_NUMBER);
	writeStream.write((char*)&AINET_VERSION_NUMBER, sizeof(int));

	int mapVersion = g_pServerGlobalVariables->m_nMapVersion;
	DevMsg(eLog::NS, "writing map version: %i\n", mapVersion);
	writeStream.write((char*)&mapVersion, sizeof(int));
	DevMsg(eLog::NS, "writing placeholder crc: %i\n", PLACEHOLDER_CRC);
	writeStream.write((char*)&PLACEHOLDER_CRC, sizeof(int));

	int calculatedLinkcount = 0;

	// path nodes
	DevMsg(eLog::NS, "writing nodecount: %i\n", aiNetwork->nodecount);
	writeStream.write((char*)&aiNetwork->nodecount, sizeof(int));

	for (int i = 0; i < aiNetwork->nodecount; i++)
	{
		// construct on-disk node struct
		CAI_NodeDisk diskNode;
		diskNode.x = aiNetwork->nodes[i]->x;
		diskNode.y = aiNetwork->nodes[i]->y;
		diskNode.z = aiNetwork->nodes[i]->z;
		diskNode.yaw = aiNetwork->nodes[i]->yaw;
		memcpy(diskNode.hulls, aiNetwork->nodes[i]->hulls, sizeof(diskNode.hulls));
		diskNode.unk0 = (char)aiNetwork->nodes[i]->unk0;
		diskNode.unk1 = aiNetwork->nodes[i]->unk1;

		for (int j = 0; j < MAX_HULLS; j++)
		{
			diskNode.unk2[j] = (short)aiNetwork->nodes[i]->unk2[j];
			DevMsg(eLog::NS, "%i\n", (short)aiNetwork->nodes[i]->unk2[j]);
		}

		memcpy(diskNode.unk3, aiNetwork->nodes[i]->unk3, sizeof(diskNode.unk3));
		diskNode.unk4 = aiNetwork->nodes[i]->unk6;
		diskNode.unk5 = -1; // aiNetwork->nodes[i]->unk8; // this field is wrong, however, it's always -1 in vanilla navmeshes anyway, so no biggie
		memcpy(diskNode.unk6, aiNetwork->nodes[i]->unk10, sizeof(diskNode.unk6));

		DevMsg(eLog::NS, "writing node %i from %li to 0x%p\n", aiNetwork->nodes[i]->index, (void*)aiNetwork->nodes[i], writeStream.tellp());
		writeStream.write((char*)&diskNode, sizeof(CAI_NodeDisk));

		calculatedLinkcount += aiNetwork->nodes[i]->linkcount;
	}

	// links
	DevMsg(eLog::NS, "linkcount: %i\n", aiNetwork->linkcount);
	DevMsg(eLog::NS, "calculated total linkcount: %i\n", calculatedLinkcount);

	calculatedLinkcount /= 2;
	if (Cvar_ns_ai_dumpAINfileFromLoad->GetBool())
	{
		if (aiNetwork->linkcount == calculatedLinkcount)
			DevMsg(eLog::NS, "caculated linkcount is normal!\n");
		else
			Warning(eLog::NS, "calculated linkcount has weird value! this is expected on build!\n");
	}

	DevMsg(eLog::NS, "writing linkcount: %i\n", calculatedLinkcount);
	writeStream.write((char*)&calculatedLinkcount, sizeof(int));

	for (int i = 0; i < aiNetwork->nodecount; i++)
	{
		for (int j = 0; j < aiNetwork->nodes[i]->linkcount; j++)
		{
			// skip links that don't originate from current node
			if (aiNetwork->nodes[i]->links[j]->srcId != aiNetwork->nodes[i]->index)
				continue;

			CAI_NodeLinkDisk diskLink;
			diskLink.srcId = aiNetwork->nodes[i]->links[j]->srcId;
			diskLink.destId = aiNetwork->nodes[i]->links[j]->destId;
			diskLink.unk0 = aiNetwork->nodes[i]->links[j]->unk1;
			memcpy(diskLink.hulls, aiNetwork->nodes[i]->links[j]->hulls, sizeof(diskLink.hulls));

			DevMsg(eLog::NS, "writing link %i => %i to 0x%p\n", diskLink.srcId, diskLink.destId, writeStream.tellp());
			writeStream.write((char*)&diskLink, sizeof(CAI_NodeLinkDisk));
		}
	}

	// don't know what this is, it's likely a block from tf1 that got deprecated? should just be 1 int per node
	DevMsg(eLog::NS, "writing %x bytes for unknown block at 0x%p\n", aiNetwork->nodecount * sizeof(uint32_t), writeStream.tellp());
	uint32_t* unkNodeBlock = new uint32_t[aiNetwork->nodecount];
	memset(unkNodeBlock, 0, aiNetwork->nodecount * sizeof(uint32_t));
	writeStream.write((char*)unkNodeBlock, aiNetwork->nodecount * sizeof(uint32_t));
	delete[] unkNodeBlock;

	// TODO: this is traverse nodes i think? these aren't used in tf2 ains so we can get away with just writing count=0 and skipping
	// but ideally should actually dump these
	DevMsg(eLog::NS, "writing 0x%p traversal nodes at {:x}...\n", 0, writeStream.tellp());
	short traverseNodeCount = 0;
	writeStream.write((char*)&traverseNodeCount, sizeof(short));
	// only write count since count=0 means we don't have to actually do anything here

	// TODO: ideally these should be actually dumped, but they're always 0 in tf2 from what i can tell
	DevMsg(eLog::NS, "writing %i bytes for unknown hull block at 0x%p\n", MAX_HULLS * 8, writeStream.tellp());
	char* unkHullBlock = new char[MAX_HULLS * 8];
	memset(unkHullBlock, 0, MAX_HULLS * 8);
	writeStream.write(unkHullBlock, MAX_HULLS * 8);
	delete[] unkHullBlock;

	// unknown struct that's seemingly node-related
	DevMsg(eLog::NS, "writing %i unknown node structs at 0x%p\n", *pUnkStruct0Count, writeStream.tellp());
	writeStream.write((char*)pUnkStruct0Count, sizeof(*pUnkStruct0Count));
	for (int i = 0; i < *pUnkStruct0Count; i++)
	{
		DevMsg(eLog::NS, "writing unknown node struct %i at 0x%p\n", i, writeStream.tellp());
		UnkNodeStruct0* nodeStruct = (*pppUnkNodeStruct0s)[i];

		writeStream.write((char*)&nodeStruct->index, sizeof(nodeStruct->index));
		writeStream.write((char*)&nodeStruct->unk1, sizeof(nodeStruct->unk1));

		writeStream.write((char*)&nodeStruct->x, sizeof(nodeStruct->x));
		writeStream.write((char*)&nodeStruct->y, sizeof(nodeStruct->y));
		writeStream.write((char*)&nodeStruct->z, sizeof(nodeStruct->z));

		writeStream.write((char*)&nodeStruct->unkcount0, sizeof(nodeStruct->unkcount0));
		for (int j = 0; j < nodeStruct->unkcount0; j++)
		{
			short unk2Short = (short)nodeStruct->unk2[j];
			writeStream.write((char*)&unk2Short, sizeof(unk2Short));
		}

		writeStream.write((char*)&nodeStruct->unkcount1, sizeof(nodeStruct->unkcount1));
		for (int j = 0; j < nodeStruct->unkcount1; j++)
		{
			short unk3Short = (short)nodeStruct->unk3[j];
			writeStream.write((char*)&unk3Short, sizeof(unk3Short));
		}

		writeStream.write((char*)&nodeStruct->unk5, sizeof(nodeStruct->unk5));
	}

	// unknown struct that's seemingly link-related
	DevMsg(eLog::NS, "writing %i unknown link structs at 0x%p\n", *pUnkLinkStruct1Count, writeStream.tellp());
	writeStream.write((char*)pUnkLinkStruct1Count, sizeof(*pUnkLinkStruct1Count));
	for (int i = 0; i < *pUnkLinkStruct1Count; i++)
	{
		// disk and memory structs are literally identical here so just directly write
		DevMsg(eLog::NS, "writing unknown link struct %i at 0x%p\n", i, writeStream.tellp());
		writeStream.write((char*)(*pppUnkStruct1s)[i], sizeof(*(*pppUnkStruct1s)[i]));
	}

	// some weird int idk what this is used for
	writeStream.write((char*)&aiNetwork->unk5, sizeof(aiNetwork->unk5));

	// tf2-exclusive stuff past this point, i.e. ain v57 only
	DevMsg(eLog::NS, "writing %i script nodes at 0x%p\n", aiNetwork->scriptnodecount, writeStream.tellp());
	writeStream.write((char*)&aiNetwork->scriptnodecount, sizeof(aiNetwork->scriptnodecount));
	for (int i = 0; i < aiNetwork->scriptnodecount; i++)
	{
		// disk and memory structs are literally identical here so just directly write
		DevMsg(eLog::NS, "writing script node %i at 0x%p\n", i, writeStream.tellp());
		writeStream.write((char*)&aiNetwork->scriptnodes[i], sizeof(aiNetwork->scriptnodes[i]));
	}

	DevMsg(eLog::NS, "writing %i hints at 0x%p\n", aiNetwork->hintcount, writeStream.tellp());
	writeStream.write((char*)&aiNetwork->hintcount, sizeof(aiNetwork->hintcount));
	for (int i = 0; i < aiNetwork->hintcount; i++)
	{
		DevMsg(eLog::NS, "writing hint data %i at 0x%p\n", i, writeStream.tellp());
		writeStream.write((char*)&aiNetwork->hints[i], sizeof(aiNetwork->hints[i]));
	}

	writeStream.close();
}

//-----------------------------------------------------------------------------
// Purpose: Draw network using debug overlay
// Input  : *pNetwork
//-----------------------------------------------------------------------------
void CAI_Helper::DrawNetwork(CAI_Network* pNetwork)
{
	if (!pNetwork)
	{
		return;
	}

	Vector3 vCamera;
	QAngle aCamera;
	float fFov;
	g_pClientTools->GetLocalPlayerEyePosition(vCamera, aCamera, fFov);

	VPlane CullPlane(vCamera - aCamera.GetNormal() * 256.0f, aCamera);

	Vector3 min(-16.0f, -16.0f, -16.0f);
	Vector3 max(16.0f, 16.0f, 16.0f);

	QAngle ang;

	for (int i = 0; i < pNetwork->scriptnodecount; i++)
	{
		CAI_ScriptNode* pNode = &pNetwork->scriptnodes[i];

		Vector3 org;
		org.x = pNode->x;
		org.y = pNode->y;
		org.z = pNode->z;

		if (CullPlane.GetPointSide(org) != SIDE_FRONT)
			continue;

		RenderWireframeBox(org, ang, min, max, Color(50, 220, 230), true, false);
	}
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
	pUnkStruct0Count = module.Offset(0x1063BF8).RCast<int*>();
	pppUnkNodeStruct0s = module.Offset(0x1063BE0).RCast<UnkNodeStruct0***>();
	pUnkLinkStruct1Count = module.Offset(0x1063AA8).RCast<int*>();
	pppUnkStruct1s = module.Offset(0x1063A90).RCast<UnkLinkStruct1***>();
}
