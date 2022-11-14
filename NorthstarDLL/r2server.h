#pragma once

#include "vector.h"

// use the R2 namespace for game funcs
namespace R2
{
	// server entity stuff
	class CBaseEntity;
	extern CBaseEntity* (*Server_GetEntityByIndex)(int index);

	// clang-format off
	OFFSET_STRUCT(CBasePlayer)
	{
		FIELD(0x58, uint32_t m_nPlayerIndex)

		FIELD(0x23E8, bool m_grappleActive)
		FIELD(0x1D08, uint32_t m_platformUserId)
		FIELD(0x1D10, int32_t m_classModsActive)
		FIELD(7564, int32_t m_posClassModsActive)
		FIELD(7628, bool m_passives)
		FIELD(4948, int32_t m_selectedOffhand)
		FIELD(4952, int32_t m_selectedOffhandPendingHybridAction)
		FIELD(7816, int32_t m_playerFlags)
		FIELD(9896, int32_t m_lastUCmdSimulationTicks)
		FIELD(9900, float m_lastUCmdSimulationRemainderTime)
		FIELD(0x1F04, int32_t m_remoteTurret)
		FIELD(0x414, int32_t m_hGroundEntity)
		FIELD(5048, int32_t m_titanSoul)
		FIELD(0x2054, int32_t m_petTitan)
		FIELD(0x4D4, int32_t m_iHealth)
		FIELD(0x4D0, int32_t m_iMaxHealth)
		FIELD(0x4F1, int32_t m_lifeState)
		FIELD(0x50C, float m_flMaxspeed)
		FIELD(0x298, int32_t m_fFlags)
		FIELD(0x1F64, int32_t m_iObserverMode)
		FIELD(0x1F6C, int32_t m_hObserverTarget)
		FIELD(0x2098, int32_t m_hViewModel)
		FIELD(10212, int32_t m_ubEFNointerpParity)
		FIELD(0x1FA4, int32_t m_activeBurnCardIndex)
		FIELD(0x1B68, int32_t m_hColorCorrectionCtrl)
		FIELD(0x19E0, int32_t m_PlayerFog__m_hCtrl)
		FIELD(9916, bool m_bShouldDrawPlayerWhileUsingViewEntity)
		FIELD(0x2848, char m_title[32])
		FIELD(10596, bool m_useCredit)
		FIELD(8000, float m_damageImpulseNoDecelEndTime)
		FIELD(0x1E8C, bool m_hasMic)
		FIELD(0x1E8D, bool m_inPartyChat)
		FIELD(0x1E90, float m_playerMoveSpeedScale)
		FIELD(0x1F58, float m_flDeathTime)
		FIELD(0x25A8, bool m_iSpawnParity)
		FIELD(1057412, Vector3 m_upDir)
		FIELD(9628, float m_lastDodgeTime)
		FIELD(0x22E0, bool m_wallHanging)
		FIELD(0x22EC, int32_t m_traversalType)
		FIELD(0x22F0, int32_t m_traversalState)
		FIELD(0x2328, Vector3 m_traversalRefPos)
		FIELD(0x231C, Vector3 m_traversalForwardDir)
		FIELD(0x2354, float m_traversalYawDelta)
		FIELD(0x2358, int32_t m_traversalYawPoseParameter)
		FIELD(0x2050, int32_t m_grappleHook)
		FIELD(0x27C0, int32_t m_autoSprintForced)
		FIELD(0x27C4, bool m_fIsSprinting)
		FIELD(0x27CC, float m_sprintStartedTime)
		FIELD(0x27D0, float m_sprintStartedFrac)
		FIELD(0x27D4, float m_sprintEndedTime)
		FIELD(0x27D8, float m_sprintEndedFrac)
		FIELD(0x27DC, float m_stickySprintStartTime)
		FIELD(10648, float m_smartAmmoPreviousHighestLockOnMeFractionValue)
		FIELD(0x23FC, int32_t m_activeZipline)
		FIELD(0x2400, bool m_ziplineReverse)
		FIELD(0x2410, int32_t m_ziplineState)
		FIELD(0x2250, int32_t m_duckState)
		FIELD(0x2254, Vector3 m_StandHullMin)
		FIELD(0x2260, Vector3 m_StandHullMax)
		FIELD(0x226C, Vector3 m_DuckHullMin)
		FIELD(0x2278, Vector3 m_DuckHullMax)
		FIELD(0x205C, int32_t m_xp)
		FIELD(0x2060, int32_t m_generation)
		FIELD(0x2064, int32_t m_rank)
		FIELD(0x2068, int32_t m_serverForceIncreasePlayerListGenerationParity)
		FIELD(0x206C, bool m_isPlayingRanked)
		FIELD(0x2070, float m_skill_mu)
		FIELD(0x1E80, int32_t m_titanSoulBeingRodeoed)
		FIELD(0x1E84, int32_t m_entitySyncingWithMe)
		FIELD(0x2078, float m_nextTitanRespawnAvailable)
		FIELD(0x1C90, bool m_hasBadReputation)
		FIELD(0x1C91, char m_communityName[64])
		FIELD(0x1CD1, char m_communityClanTag[16])
		FIELD(0x1CE1, char m_factionName[16])
		FIELD(0x1CF1, char m_hardwareIcon[16])
		FIELD(0x1D01, bool m_happyHourActive)
		FIELD(0x1EF4, int32_t m_gestureAutoKillBitfield)
		FIELD(0x2EA8, int32_t m_pilotClassIndex)
		FIELD(0x100490, Vector3 m_vecAbsOrigin)
		FIELD(0x25BE, bool m_isPerformingBoostAction)
		FIELD(0x240C, bool m_ziplineValid3pWeaponLayerAnim)
		FIELD(0x345C, int32_t m_playerScriptNetDataGlobal)
		FIELD(0x1598, int32_t m_bZooming)
		FIELD(0x1599, bool m_zoomToggleOn)
		FIELD(0x159C, float m_zoomBaseFrac)
		FIELD(0x15A0, float m_zoomBaseTime)
		FIELD(0x15A4, float m_zoomFullStartTime)
		FIELD(0xA04, int32_t m_camoIndex)
		FIELD(0xA08, int32_t m_decalIndex)
	};
	// clang-format on

	extern CBasePlayer*(__fastcall* UTIL_PlayerByIndex)(int playerIndex);
} // namespace R2
