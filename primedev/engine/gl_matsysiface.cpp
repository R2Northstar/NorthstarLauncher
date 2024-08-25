#include "materialsystem/cmaterialglue.h"

CMaterialGlue* (*GetMaterialAtCrossHair)();

AUTOHOOK_INIT()

AUTOHOOK(CC_mat_crosshair_printmaterial_f, engine.dll + 0xB3C40, void, __fastcall, (const CCommand& args))
{
	CMaterialGlue* pMat = GetMaterialAtCrossHair();

	if (!pMat)
	{
		spdlog::error("Not looking at a material!");
		return;
	}

	std::function<void(CMaterialGlue * pGlue, const char* szName)> fnPrintGlue = [](CMaterialGlue* pGlue, const char* szName)
	{
		spdlog::info("|-----------------------------------------------");

		if (!pGlue)
		{
			spdlog::info("|-- {} is NULL", szName);
			return;
		}

		spdlog::info("|-- Name: {}", szName);
		spdlog::info("|-- GUID: {:#x}", pGlue->m_GUID);
		spdlog::info("|-- Name: {}", pGlue->m_pszName);
		spdlog::info("|-- Width : {}", pGlue->m_iWidth);
		spdlog::info("|-- Height: {}", pGlue->m_iHeight);
	};

	spdlog::info("|- GUID: {:#x}", pMat->m_GUID);
	spdlog::info("|- Name: {}", pMat->m_pszName);
	spdlog::info("|- Width : {}", pMat->m_iWidth);
	spdlog::info("|- Height: {}", pMat->m_iHeight);

	fnPrintGlue(pMat->m_pDepthShadow, "DepthShadow");
	fnPrintGlue(pMat->m_pDepthPrepass, "DepthPrepass");
	fnPrintGlue(pMat->m_pDepthVSM, "DepthVSM");
	fnPrintGlue(pMat->m_pColPass, "ColPass");
}

ON_DLL_LOAD("engine.dll", GlMatSysIFace, (CModule module))
{
	AUTOHOOK_DISPATCH()

	GetMaterialAtCrossHair = module.Offset(0xB37D0).RCast<CMaterialGlue* (*)()>();
}