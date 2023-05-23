#include "bits.h"
#include "cvar.h"
#include "convar.h"
#include "core/sourceinterface.h"

#include "plugins/pluginbackend.h"

#include <float.h>

typedef void (*ConVarRegisterType)(
	ConVar* pConVar,
	const char* pszName,
	const char* pszDefaultValue,
	int nFlags,
	const char* pszHelpString,
	bool bMin,
	float fMin,
	bool bMax,
	float fMax,
	void* pCallback);
ConVarRegisterType conVarRegister;

typedef void (*ConVarMallocType)(void* pConVarMaloc, int a2, int a3);
ConVarMallocType conVarMalloc;

void* g_pConVar_Vtable = nullptr;
void* g_pIConVar_Vtable = nullptr;

//-----------------------------------------------------------------------------
// Purpose: ConVar interface initialization
//-----------------------------------------------------------------------------
ON_DLL_LOAD("engine.dll", ConVar, (CModule module))
{
	conVarMalloc = module.Offset(0x415C20).As<ConVarMallocType>();
	conVarRegister = module.Offset(0x417230).As<ConVarRegisterType>();

	g_pConVar_Vtable = module.Offset(0x67FD28);
	g_pIConVar_Vtable = module.Offset(0x67FDC8);

	R2::g_pCVarInterface = new SourceInterface<CCvar>("vstdlib.dll", "VEngineCvar007");
	R2::g_pCVar = *R2::g_pCVarInterface;

	g_pPluginCommunicationhandler->m_sEngineData.conVarMalloc = conVarMalloc;
	g_pPluginCommunicationhandler->m_sEngineData.conVarRegister = conVarRegister;
	g_pPluginCommunicationhandler->m_sEngineData.ConVar_Vtable = g_pConVar_Vtable;
	g_pPluginCommunicationhandler->m_sEngineData.IConVar_Vtable = g_pIConVar_Vtable;
}

//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
ConVar::ConVar(const char* pszName, const char* pszDefaultValue, int nFlags, const char* pszHelpString)
{
	spdlog::info("Registering Convar {}", pszName);

	this->m_ConCommandBase.m_pConCommandBaseVTable = g_pConVar_Vtable;
	this->m_ConCommandBase.s_pConCommandBases = (ConCommandBase*)g_pIConVar_Vtable;

	conVarMalloc(&this->m_pMalloc, 0, 0); // Allocate new memory for ConVar.
	conVarRegister(this, pszName, pszDefaultValue, nFlags, pszHelpString, 0, 0, 0, 0, 0);
}

//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
ConVar::ConVar(
	const char* pszName,
	const char* pszDefaultValue,
	int nFlags,
	const char* pszHelpString,
	bool bMin,
	float fMin,
	bool bMax,
	float fMax,
	FnChangeCallback_t pCallback)
{
	spdlog::info("Registering Convar {}", pszName);

	this->m_ConCommandBase.m_pConCommandBaseVTable = g_pConVar_Vtable;
	this->m_ConCommandBase.s_pConCommandBases = (ConCommandBase*)g_pIConVar_Vtable;

	conVarMalloc(&this->m_pMalloc, 0, 0); // Allocate new memory for ConVar.
	conVarRegister(this, pszName, pszDefaultValue, nFlags, pszHelpString, bMin, fMin, bMax, fMax, pCallback);
}

//-----------------------------------------------------------------------------
// Purpose: destructor
//-----------------------------------------------------------------------------
ConVar::~ConVar(void)
{
	if (m_Value.m_pszString)
		delete[] m_Value.m_pszString;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the base ConVar name.
// Output : const char*
//-----------------------------------------------------------------------------
const char* ConVar::GetBaseName(void) const
{
	return m_ConCommandBase.m_pszName;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the ConVar help text.
// Output : const char*
//-----------------------------------------------------------------------------
const char* ConVar::GetHelpText(void) const
{
	return m_ConCommandBase.m_pszHelpString;
}

//-----------------------------------------------------------------------------
// Purpose: Add's flags to ConVar.
// Input  : nFlags -
//-----------------------------------------------------------------------------
void ConVar::AddFlags(int nFlags)
{
	m_ConCommandBase.m_nFlags |= nFlags;
}

//-----------------------------------------------------------------------------
// Purpose: Removes flags from ConVar.
// Input  : nFlags -
//-----------------------------------------------------------------------------
void ConVar::RemoveFlags(int nFlags)
{
	m_ConCommandBase.m_nFlags &= ~nFlags;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a boolean.
// Output : bool
//-----------------------------------------------------------------------------
bool ConVar::GetBool(void) const
{
	return !!GetInt();
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a float.
// Output : float
//-----------------------------------------------------------------------------
float ConVar::GetFloat(void) const
{
	return m_Value.m_fValue;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as an integer.
// Output : int
//-----------------------------------------------------------------------------
int ConVar::GetInt(void) const
{
	return m_Value.m_nValue;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a color.
// Output : Color
//-----------------------------------------------------------------------------
Color ConVar::GetColor(void) const
{
	unsigned char* pColorElement = ((unsigned char*)&m_Value.m_nValue);
	return Color(pColorElement[0], pColorElement[1], pColorElement[2], pColorElement[3]);
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a string.
// Output : const char *
//-----------------------------------------------------------------------------
const char* ConVar::GetString(void) const
{
	if (m_ConCommandBase.m_nFlags & FCVAR_NEVER_AS_STRING)
	{
		return "FCVAR_NEVER_AS_STRING";
	}

	char const* str = m_Value.m_pszString;
	return str ? str : "";
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : flMinVal -
// Output : true if there is a min set.
//-----------------------------------------------------------------------------
bool ConVar::GetMin(float& flMinVal) const
{
	flMinVal = m_fMinVal;
	return m_bHasMin;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : flMaxVal -
// Output : true if there is a max set.
//-----------------------------------------------------------------------------
bool ConVar::GetMax(float& flMaxVal) const
{
	flMaxVal = m_fMaxVal;
	return m_bHasMax;
}

//-----------------------------------------------------------------------------
// Purpose: returns the min value.
// Output : float
//-----------------------------------------------------------------------------
float ConVar::GetMinValue(void) const
{
	return m_fMinVal;
}

//-----------------------------------------------------------------------------
// Purpose: returns the max value.
// Output : float
//-----------------------------------------------------------------------------
float ConVar::GetMaxValue(void) const
{
	return m_fMaxVal;
	;
}

//-----------------------------------------------------------------------------
// Purpose: checks if ConVar has min value.
// Output : bool
//-----------------------------------------------------------------------------
bool ConVar::HasMin(void) const
{
	return m_bHasMin;
}

//-----------------------------------------------------------------------------
// Purpose: checks if ConVar has max value.
// Output : bool
//-----------------------------------------------------------------------------
bool ConVar::HasMax(void) const
{
	return m_bHasMax;
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar int value.
// Input  : nValue -
//-----------------------------------------------------------------------------
void ConVar::SetValue(int nValue)
{
	if (nValue == m_Value.m_nValue)
	{
		return;
	}

	float flValue = (float)nValue;

	// Check bounds.
	if (ClampValue(flValue))
	{
		nValue = (int)(flValue);
	}

	// Redetermine value.
	float flOldValue = m_Value.m_fValue;
	m_Value.m_fValue = flValue;
	m_Value.m_nValue = nValue;

	if (!(m_ConCommandBase.m_nFlags & FCVAR_NEVER_AS_STRING))
	{
		char szTempValue[32];
		snprintf(szTempValue, sizeof(szTempValue), "%d", m_Value.m_nValue);
		ChangeStringValue(szTempValue, flOldValue);
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar float value.
// Input  : flValue -
//-----------------------------------------------------------------------------
void ConVar::SetValue(float flValue)
{
	if (flValue == m_Value.m_fValue)
	{
		return;
	}

	// Check bounds.
	ClampValue(flValue);

	// Redetermine value.
	float flOldValue = m_Value.m_fValue;
	m_Value.m_fValue = flValue;
	m_Value.m_nValue = (int)m_Value.m_fValue;

	if (!(m_ConCommandBase.m_nFlags & FCVAR_NEVER_AS_STRING))
	{
		char szTempValue[32];
		snprintf(szTempValue, sizeof(szTempValue), "%f", m_Value.m_fValue);
		ChangeStringValue(szTempValue, flOldValue);
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar string value.
// Input  : *szValue -
//-----------------------------------------------------------------------------
void ConVar::SetValue(const char* pszValue)
{
	if (strcmp(this->m_Value.m_pszString, pszValue) == 0)
		return;

	char szTempValue[32] {};
	const char* pszNewValue {};

	float flOldValue = m_Value.m_fValue;
	pszNewValue = (char*)pszValue;
	if (!pszNewValue)
	{
		pszNewValue = "";
	}

	if (!SetColorFromString(pszValue))
	{
		// Not a color, do the standard thing
		float flNewValue = (float)atof(pszValue);
		if (!isfinite(flNewValue))
		{
			spdlog::warn("Warning: ConVar '{}' = '{}' is infinite, clamping value.\n", GetBaseName(), pszValue);
			flNewValue = FLT_MAX;
		}

		if (ClampValue(flNewValue))
		{
			snprintf(szTempValue, sizeof(szTempValue), "%f", flNewValue);
			pszNewValue = szTempValue;
		}

		// Redetermine value
		m_Value.m_fValue = flNewValue;
		m_Value.m_nValue = (int)(m_Value.m_fValue);
	}

	if (!(m_ConCommandBase.m_nFlags & FCVAR_NEVER_AS_STRING))
	{
		ChangeStringValue(pszNewValue, flOldValue);
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar color value.
// Input  : clValue -
//-----------------------------------------------------------------------------
void ConVar::SetValue(Color clValue)
{
	std::string svResult = "";

	for (int i = 0; i < 4; i++)
	{
		if (!(clValue.GetValue(i) == 0 && svResult.size() == 0))
		{
			svResult += std::to_string(clValue.GetValue(i));
			svResult.append(" ");
		}
	}

	this->m_Value.m_pszString = svResult.c_str();
}

//-----------------------------------------------------------------------------
// Purpose: changes the ConVar string value.
// Input  : *pszTempVal - flOldValue
//-----------------------------------------------------------------------------
void ConVar::ChangeStringValue(const char* pszTempVal, float flOldValue)
{
	assert(!(m_ConCommandBase.m_nFlags & FCVAR_NEVER_AS_STRING));

	char* pszOldValue = (char*)_malloca(m_Value.m_iStringLength);
	if (pszOldValue != NULL)
	{
		memcpy(pszOldValue, m_Value.m_pszString, m_Value.m_iStringLength);
	}

	if (pszTempVal)
	{
		int len = strlen(pszTempVal) + 1;

		if (len > m_Value.m_iStringLength)
		{
			if (m_Value.m_pszString)
				delete[] m_Value.m_pszString;

			m_Value.m_pszString = new char[len];
			m_Value.m_iStringLength = len;
		}

		memcpy((char*)m_Value.m_pszString, pszTempVal, len);
	}
	else
	{
		m_Value.m_pszString = NULL;
	}

	pszOldValue = 0;
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar color value from string.
// Input  : *pszValue -
//-----------------------------------------------------------------------------
bool ConVar::SetColorFromString(const char* pszValue)
{
	bool bColor = false;

	// Try pulling RGBA color values out of the string.
	int nRGBA[4] {};
	int nParamsRead = sscanf_s(pszValue, "%i %i %i %i", &(nRGBA[0]), &(nRGBA[1]), &(nRGBA[2]), &(nRGBA[3]));

	if (nParamsRead >= 3)
	{
		// This is probably a color!
		if (nParamsRead == 3)
		{
			// Assume they wanted full alpha.
			nRGBA[3] = 255;
		}

		if (nRGBA[0] >= 0 && nRGBA[0] <= 255 && nRGBA[1] >= 0 && nRGBA[1] <= 255 && nRGBA[2] >= 0 && nRGBA[2] <= 255 && nRGBA[3] >= 0 &&
			nRGBA[3] <= 255)
		{
			// printf("*** WOW! Found a color!! ***\n");

			// This is definitely a color!
			bColor = true;

			// Stuff all the values into each byte of our int.
			unsigned char* pColorElement = ((unsigned char*)&m_Value.m_nValue);
			pColorElement[0] = nRGBA[0];
			pColorElement[1] = nRGBA[1];
			pColorElement[2] = nRGBA[2];
			pColorElement[3] = nRGBA[3];

			// Copy that value into our float.
			m_Value.m_fValue = (float)(m_Value.m_nValue);
		}
	}

	return bColor;
}

//-----------------------------------------------------------------------------
// Purpose: Checks if ConVar is registered.
// Output : bool
//-----------------------------------------------------------------------------
bool ConVar::IsRegistered(void) const
{
	return m_ConCommandBase.m_bRegistered;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if this is a command
// Output : bool
//-----------------------------------------------------------------------------
bool ConVar::IsCommand(void) const
{
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Test each ConVar query before setting the value.
// Input  : nFlags
// Output : False if change is permitted, true if not.
//-----------------------------------------------------------------------------
bool ConVar::IsFlagSet(int nFlags) const
{
	return m_ConCommandBase.m_nFlags & nFlags;
}

//-----------------------------------------------------------------------------
// Purpose: Check whether to clamp and then perform clamp.
// Input  : flValue -
// Output : Returns true if value changed.
//-----------------------------------------------------------------------------
bool ConVar::ClampValue(float& flValue)
{
	if (m_bHasMin && (flValue < m_fMinVal))
	{
		flValue = m_fMinVal;
		return true;
	}

	if (m_bHasMax && (flValue > m_fMaxVal))
	{
		flValue = m_fMaxVal;
		return true;
	}

	return false;
}

int ParseConVarFlagsString(std::string modName, std::string sFlags)
{
	int iFlags = 0;
	std::stringstream stFlags(sFlags);
	std::string sFlag;

	while (std::getline(stFlags, sFlag, '|'))
	{
		// trim the flag
		sFlag.erase(sFlag.find_last_not_of(" \t\n\f\v\r") + 1);
		sFlag.erase(0, sFlag.find_first_not_of(" \t\n\f\v\r"));

		// skip if empty
		if (sFlag.empty())
			continue;

		// find the matching flag value
		bool ok = false;
		for (auto const& flagPair : g_PrintCommandFlags)
		{
			if (sFlag == flagPair.second)
			{
				iFlags |= flagPair.first;
				ok = true;
				break;
			}
		}
		if (!ok)
		{
			spdlog::warn("Mod ConCommand {} has unknown flag {}", modName, sFlag);
		}
	}

	return iFlags;
}
