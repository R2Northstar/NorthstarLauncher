#include "pch.h"
#include "hooks.h"
#include "scriptmainmenupromos.h"
#include "squirrel.h"
#include "masterserver.h"

// mirror this in script
enum eMainMenuPromoDataProperty
{
	newInfoTitle1,
	newInfoTitle2,
	newInfoTitle3,

	largeButtonTitle,
	largeButtonText,
	largeButtonUrl,
	largeButtonImageIndex,

	smallButton1Title,
	smallButton1Url,
	smallButton1ImageIndex,

	smallButton2Title,
	smallButton2Url,
	smallButton2ImageIndex
};

// void function NSRequestCustomMainMenuPromos()
SQRESULT SQ_RequestCustomMainMenuPromos(void* sqvm)
{
	g_MasterServerManager->RequestMainMenuPromos();
	return SQRESULT_NULL;
}

// bool function NSHasCustomMainMenuPromoData()
SQRESULT SQ_HasCustomMainMenuPromoData(void* sqvm)
{
	g_pUISquirrel->pushbool(sqvm, g_MasterServerManager->m_bHasMainMenuPromoData);
	return SQRESULT_NOTNULL;
}

// var function NSGetCustomMainMenuPromoData( int promoDataKey )
SQRESULT SQ_GetCustomMainMenuPromoData(void* sqvm)
{
	if (!g_MasterServerManager->m_bHasMainMenuPromoData)
		return SQRESULT_NULL;

	switch (g_pUISquirrel->getinteger(sqvm, 1))
	{
	case eMainMenuPromoDataProperty::newInfoTitle1:
	{
		g_pUISquirrel->pushstring(sqvm, g_MasterServerManager->m_sMainMenuPromoData.newInfoTitle1.c_str());
		break;
	}

	case eMainMenuPromoDataProperty::newInfoTitle2:
	{
		g_pUISquirrel->pushstring(sqvm, g_MasterServerManager->m_sMainMenuPromoData.newInfoTitle2.c_str());
		break;
	}

	case eMainMenuPromoDataProperty::newInfoTitle3:
	{
		g_pUISquirrel->pushstring(sqvm, g_MasterServerManager->m_sMainMenuPromoData.newInfoTitle3.c_str());
		break;
	}

	case eMainMenuPromoDataProperty::largeButtonTitle:
	{
		g_pUISquirrel->pushstring(sqvm, g_MasterServerManager->m_sMainMenuPromoData.largeButtonTitle.c_str());
		break;
	}

	case eMainMenuPromoDataProperty::largeButtonText:
	{
		g_pUISquirrel->pushstring(sqvm, g_MasterServerManager->m_sMainMenuPromoData.largeButtonText.c_str());
		break;
	}

	case eMainMenuPromoDataProperty::largeButtonUrl:
	{
		g_pUISquirrel->pushstring(sqvm, g_MasterServerManager->m_sMainMenuPromoData.largeButtonUrl.c_str());
		break;
	}

	case eMainMenuPromoDataProperty::largeButtonImageIndex:
	{
		g_pUISquirrel->pushinteger(sqvm, g_MasterServerManager->m_sMainMenuPromoData.largeButtonImageIndex);
		break;
	}

	case eMainMenuPromoDataProperty::smallButton1Title:
	{
		g_pUISquirrel->pushstring(sqvm, g_MasterServerManager->m_sMainMenuPromoData.smallButton1Title.c_str());
		break;
	}

	case eMainMenuPromoDataProperty::smallButton1Url:
	{
		g_pUISquirrel->pushstring(sqvm, g_MasterServerManager->m_sMainMenuPromoData.smallButton1Url.c_str());
		break;
	}

	case eMainMenuPromoDataProperty::smallButton1ImageIndex:
	{
		g_pUISquirrel->pushinteger(sqvm, g_MasterServerManager->m_sMainMenuPromoData.smallButton1ImageIndex);
		break;
	}

	case eMainMenuPromoDataProperty::smallButton2Title:
	{
		g_pUISquirrel->pushstring(sqvm, g_MasterServerManager->m_sMainMenuPromoData.smallButton2Title.c_str());
		break;
	}

	case eMainMenuPromoDataProperty::smallButton2Url:
	{
		g_pUISquirrel->pushstring(sqvm, g_MasterServerManager->m_sMainMenuPromoData.smallButton2Url.c_str());
		break;
	}

	case eMainMenuPromoDataProperty::smallButton2ImageIndex:
	{
		g_pUISquirrel->pushinteger(sqvm, g_MasterServerManager->m_sMainMenuPromoData.smallButton2ImageIndex);
		break;
	}
	}

	return SQRESULT_NOTNULL;
}

ON_DLL_LOAD_CLIENT_RELIESON("client.dll", ScriptMainMenuPrograms, ClientSquirrel, [](HMODULE baseAddress)
{
	g_pUISquirrel->AddFuncRegistration("void", "NSRequestCustomMainMenuPromos", "", "", SQ_RequestCustomMainMenuPromos);
	g_pUISquirrel->AddFuncRegistration("bool", "NSHasCustomMainMenuPromoData", "", "", SQ_HasCustomMainMenuPromoData);
	g_pUISquirrel->AddFuncRegistration("var", "NSGetCustomMainMenuPromoData", "int promoDataKey", "", SQ_GetCustomMainMenuPromoData);
})