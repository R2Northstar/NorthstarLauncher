#include "pch.h"
#include "bits.h"
#include "cvar.h"
#include "convar.h"
#include "hookutils.h"
#include "gameutils.h"
#include "sourceinterface.h"

// should this be in modmanager?
std::unordered_map<std::string, ConVar*> g_CustomConvars; // this is used in modloading code to determine whether we've registered a mod convar already

typedef void (*ConVarConstructorType)(ConVar* pNewVar, const char* name, const char* defaultValue, int flags, const char* helpString);
ConVarConstructorType conVarConstructor;

typedef void (*ConVarRegisterType)(
	ConVar* pConVar, const char* pszName, const char* pszDefaultValue, int nFlags, const char* pszHelpString, bool bMin, float fMin,
	bool bMax, float fMax, void* pCallback);
ConVarRegisterType conVarRegister;

typedef void (*ConVarMallocType)(void* pConVarMaloc, int a2, int a3);
ConVarMallocType conVarMalloc;

void* g_pConVar_Vtable  = nullptr;
void* g_pIConVar_Vtable = nullptr;

typedef bool (*CvarIsFlagSetType)(ConVar* self, int flags);
CvarIsFlagSetType CvarIsFlagSet;

//-----------------------------------------------------------------------------
// Purpose: ConVar interface initialization
//-----------------------------------------------------------------------------
void InitialiseConVars(HMODULE baseAddress)
{
	conVarConstructor = (ConVarConstructorType)((char*)baseAddress + 0x416200);
	conVarMalloc = (ConVarMallocType)((char*)baseAddress + 0x415C20);
	conVarRegister = (ConVarRegisterType)((char*)baseAddress + 0x417230);

	g_pConVar_Vtable = (char*)baseAddress + 0x67FD28;
	g_pIConVar_Vtable = (char*)baseAddress + 0x67FDC8;

	g_pCVarInterface = new SourceInterface<CCvar>("vstdlib.dll", "VEngineCvar007");
	g_pCVar = *g_pCVarInterface;

	HookEnabler hook;
	ENABLER_CREATEHOOK(hook, (char*)baseAddress + 0x417FA0, &ConVar::IsFlagSet, reinterpret_cast<LPVOID*>(&CvarIsFlagSet));
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

	g_CustomConvars.emplace(pszName, this);
}

//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
ConVar::ConVar(
	const char* pszName, const char* pszDefaultValue, int nFlags, const char* pszHelpString, bool bMin, float fMin, bool bMax, float fMax,
	void* pCallback)
{
	spdlog::info("Registering Convar {}", pszName);

	this->m_ConCommandBase.m_pConCommandBaseVTable = g_pConVar_Vtable;
	this->m_ConCommandBase.s_pConCommandBases = (ConCommandBase*)g_pIConVar_Vtable;

	conVarMalloc(&this->m_pMalloc, 0, 0); // Allocate new memory for ConVar.
	conVarRegister(this, pszName, pszDefaultValue, nFlags, pszHelpString, bMin, fMin, bMax, fMax, pCallback);

	g_CustomConvars.emplace(pszName, this);
}

//-----------------------------------------------------------------------------
// Purpose: destructor
//-----------------------------------------------------------------------------
ConVar::~ConVar(void)
{
	if (m_pzsCurrentValue)
	{
		delete[] m_pzsCurrentValue;
		m_pzsCurrentValue = NULL;
	}
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
// Purpose: Checks if ConVar is registered.
// Output : bool
//-----------------------------------------------------------------------------
bool ConVar::IsRegistered(void) const
{
	return m_ConCommandBase.m_bRegistered;
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
	return m_flValue;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as an integer.
// Output : int
//-----------------------------------------------------------------------------
int ConVar::GetInt(void) const
{
	return m_nValue;
}

//-----------------------------------------------------------------------------
// Purpose: Return ConVar value as a color.
// Output : Color
//-----------------------------------------------------------------------------
Color ConVar::GetColor(void) const
{
	unsigned char* pColorElement = ((unsigned char*)&m_nValue);
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

	char const* str = m_pzsCurrentValue;
	return str ? str : "";
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flMinVal - 
// Output : true if there is a min set.
//-----------------------------------------------------------------------------
bool ConVar::GetMin(float& flMinVal) const
{
	flMinVal = m_flMinValue;
	return m_bHasMin;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flMaxVal - 
// Output : true if there is a max set.
//-----------------------------------------------------------------------------
bool ConVar::GetMax(float& flMaxVal) const
{
	flMaxVal = m_flMaxValue;
	return m_bHasMax;
}

//-----------------------------------------------------------------------------
// Purpose: returns the min value.
// Output : float
//-----------------------------------------------------------------------------
float ConVar::GetMinValue(void) const
{
	return m_flMinValue;
}

//-----------------------------------------------------------------------------
// Purpose: returns the max value.
// Output : float
//-----------------------------------------------------------------------------
float ConVar::GetMaxValue(void) const
{
	return m_flMaxValue;;
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
	if (nValue == m_nValue)
	{
		return;
	}

	// Only valid for root ConVars.
	assert(m_pParent == this);

	float flValue = (float)nValue;

	// Check bounds.
	if (ClampValue(flValue))
	{
		nValue = (int)(flValue);
	}

	// Redetermine value.
	float flOldValue = m_flValue;
	m_flValue = flValue;
	m_nValue = nValue;

	if (!(m_ConCommandBase.m_nFlags & FCVAR_NEVER_AS_STRING))
	{
		char szTempValue[32];
		snprintf(szTempValue, sizeof(szTempValue), "%d", m_nValue);
		ChangeStringValue(szTempValue, flOldValue);
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar float value.
// Input  : flValue - 
//-----------------------------------------------------------------------------
void ConVar::SetValue(float flValue)
{
	if (flValue == m_flValue)
	{
		return;
	}

	// Only valid for root ConVars.
	assert(m_pParent == this);

	// Check bounds.
	ClampValue(flValue);

	// Redetermine value.
	float flOldValue = m_flValue;
	m_flValue = flValue;
	m_nValue = (int)m_flValue;

	if (!(m_ConCommandBase.m_nFlags & FCVAR_NEVER_AS_STRING))
	{
		char szTempValue[32];
		snprintf(szTempValue, sizeof(szTempValue), "%f", m_flValue);
		ChangeStringValue(szTempValue, flOldValue);
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets the ConVar string value.
// Input  : *szValue - 
//-----------------------------------------------------------------------------
void ConVar::SetValue(const char* pszValue)
{
	if (strcmp(this->m_pzsCurrentValue, pszValue) == 0)
	{
		return;
	}
	this->m_pzsCurrentValue = pszValue;

	char szTempValue[32]{};
	const char* pszNewValue{};

	// Only valid for root convars.
	assert(m_pParent == this);

	float flOldValue = m_flValue;
	pszNewValue = (char*)pszValue;
	if (!pszNewValue)
	{
		pszNewValue = "";
	}

	if (!SetColorFromString(pszValue))
	{
		// Not a color, do the standard thing
		float flNewValue = (float)atof(pszValue);
		if (!IsFinite(flNewValue))
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
		m_flValue = flNewValue;
		m_nValue = (int)(m_flValue);
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

	this->m_pzsCurrentValue = svResult.c_str();
}

//-----------------------------------------------------------------------------
// Purpose: changes the ConVar string value.
// Input  : *pszTempVal - flOldValue
//-----------------------------------------------------------------------------
void ConVar::ChangeStringValue(const char* pszTempVal, float flOldValue)
{
	assert(!(m_ConCommandBase.m_nFlags & FCVAR_NEVER_AS_STRING));

	char* pszOldValue = (char*)_malloca(m_iStringLength);
	if (pszOldValue != NULL)
	{
		memcpy(pszOldValue, m_pzsCurrentValue, m_iStringLength);
	}

	if (pszTempVal)
	{
		int len = strlen(pszTempVal) + 1;

		if (len > m_iStringLength)
		{
			if (m_pzsCurrentValue)
			{
				delete[] m_pzsCurrentValue;
			}

			m_pzsCurrentValue = new char[len];
			m_iStringLength = len;
		}

		memcpy((char*)m_pzsCurrentValue, pszTempVal, len);
	}
	else
	{
		m_pzsCurrentValue = NULL;
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
	int nRGBA[4]{};
	int nParamsRead = sscanf_s(pszValue, "%i %i %i %i", &(nRGBA[0]), &(nRGBA[1]), &(nRGBA[2]), &(nRGBA[3]));

	if (nParamsRead >= 3)
	{
		// This is probably a color!
		if (nParamsRead == 3)
		{
			// Assume they wanted full alpha.
			nRGBA[3] = 255;
		}

		if (nRGBA[0] >= 0 && nRGBA[0] <= 255 &&
			nRGBA[1] >= 0 && nRGBA[1] <= 255 &&
			nRGBA[2] >= 0 && nRGBA[2] <= 255 &&
			nRGBA[3] >= 0 && nRGBA[3] <= 255)
		{
			//printf("*** WOW! Found a color!! ***\n");

			// This is definitely a color!
			bColor = true;

			// Stuff all the values into each byte of our int.
			unsigned char* pColorElement = ((unsigned char*)&m_nValue);
			pColorElement[0] = nRGBA[0];
			pColorElement[1] = nRGBA[1];
			pColorElement[2] = nRGBA[2];
			pColorElement[3] = nRGBA[3];

			// Copy that value into our float.
			m_flValue = (float)(m_nValue);
		}
	}

	return bColor;
}

//-----------------------------------------------------------------------------
// Purpose: Test each ConVar query before setting the value.
// Input  : *pConVar - nFlags
// Output : False if change is permitted, true if not.
//-----------------------------------------------------------------------------
bool ConVar::IsFlagSet(ConVar* pConVar, int nFlags)
{
	// unrestrict FCVAR_DEVELOPMENTONLY and FCVAR_HIDDEN
	if (pConVar && (nFlags == FCVAR_DEVELOPMENTONLY || nFlags == FCVAR_HIDDEN))
		return false;

	return CvarIsFlagSet(pConVar, nFlags);
}

//-----------------------------------------------------------------------------
// Purpose: Check whether to clamp and then perform clamp.
// Input  : flValue - 
// Output : Returns true if value changed.
//-----------------------------------------------------------------------------
bool ConVar::ClampValue(float& flValue)
{
	if (m_bHasMin && (flValue < m_flMinValue))
	{
		flValue = m_flMinValue;
		return true;
	}

	if (m_bHasMax && (flValue > m_flMaxValue))
	{
		flValue = m_flMaxValue;
		return true;
	}

	return false;
}
