#include "dedicated/dedicated.h"
#include "core/convar/cvar.h"
#include "core/math/vector.h"

AUTOHOOK_INIT()

enum OverlayType_t
{
	OVERLAY_BOX = 0,
	OVERLAY_SPHERE,
	OVERLAY_LINE,
	OVERLAY_TRIANGLE,
	OVERLAY_SWEPT_BOX,
	OVERLAY_BOX2,
	OVERLAY_CAPSULE
};

struct OverlayBase_t
{
	OverlayBase_t()
	{
		m_Type = OVERLAY_BOX;
		m_nServerCount = -1;
		m_nCreationTick = -1;
		m_flEndTime = 0.0f;
		m_pNextOverlay = NULL;
	}

	OverlayType_t m_Type; // What type of overlay is it?
	int m_nCreationTick; // Duration -1 means go away after this frame #
	int m_nServerCount; // Latch server count, too
	float m_flEndTime; // When does this box go away
	OverlayBase_t* m_pNextOverlay;
	void* m_pUnk;
};

struct OverlayLine_t : public OverlayBase_t
{
	OverlayLine_t()
	{
		m_Type = OVERLAY_LINE;
	}

	Vector3 origin;
	Vector3 dest;
	int r;
	int g;
	int b;
	int a;
	bool noDepthTest;
};

struct OverlayBox_t : public OverlayBase_t
{
	OverlayBox_t()
	{
		m_Type = OVERLAY_BOX;
	}

	Vector3 origin;
	Vector3 mins;
	Vector3 maxs;
	QAngle angles;
	int r;
	int g;
	int b;
	int a;
};

static HMODULE sEngineModule;

typedef void (*RenderLineType)(Vector3 v1, Vector3 v2, Color c, bool bZBuffer);
static RenderLineType RenderLine;
typedef void (*RenderBoxType)(Vector3 vOrigin, QAngle angles, Vector3 vMins, Vector3 vMaxs, Color c, bool bZBuffer, bool bInsideOut);
static RenderBoxType RenderBox;
static RenderBoxType RenderWireframeBox;

typedef bool (*OverlayBase_t__IsDeadType)(OverlayBase_t* a1);
static OverlayBase_t__IsDeadType OverlayBase_t__IsDead;
typedef void (*OverlayBase_t__DestroyOverlayType)(OverlayBase_t* a1);
static OverlayBase_t__DestroyOverlayType OverlayBase_t__DestroyOverlay;

static ConVar* Cvar_enable_debug_overlays;

// clang-format off
AUTOHOOK(DrawOverlay, engine.dll + 0xABCB0, 
void, __fastcall, (OverlayBase_t * pOverlay))
// clang-format on
{
	EnterCriticalSection((LPCRITICAL_SECTION)((char*)sEngineModule + 0x10DB0A38)); // s_OverlayMutex

	void* pMaterialSystem = *(void**)((char*)sEngineModule + 0x14C675B0);

	switch (pOverlay->m_Type)
	{
	case OVERLAY_LINE:
	{
		OverlayLine_t* pLine = static_cast<OverlayLine_t*>(pOverlay);
		RenderLine(pLine->origin, pLine->dest, Color(pLine->r, pLine->g, pLine->b, pLine->a), pLine->noDepthTest);
	}
	break;
	case OVERLAY_BOX:
	{
		OverlayBox_t* pCurrBox = static_cast<OverlayBox_t*>(pOverlay);
		if (pCurrBox->a > 0)
		{
			RenderBox(
				pCurrBox->origin,
				pCurrBox->angles,
				pCurrBox->mins,
				pCurrBox->maxs,
				Color(pCurrBox->r, pCurrBox->g, pCurrBox->b, pCurrBox->a),
				false,
				false);
		}
		if (pCurrBox->a < 255)
		{
			RenderWireframeBox(
				pCurrBox->origin,
				pCurrBox->angles,
				pCurrBox->mins,
				pCurrBox->maxs,
				Color(pCurrBox->r, pCurrBox->g, pCurrBox->b, 255),
				false,
				false);
		}
	}
	break;
	default:
		DrawOverlay(pOverlay);
		break;
	}
	LeaveCriticalSection((LPCRITICAL_SECTION)((char*)sEngineModule + 0x10DB0A38));
}

// clang-format off
AUTOHOOK(DrawAllOverlays, engine.dll + 0xAB780, 
void, __fastcall, (bool bRender))
// clang-format on
{
	EnterCriticalSection((LPCRITICAL_SECTION)((char*)sEngineModule + 0x10DB0A38)); // s_OverlayMutex

	// vanilla checks the convar first, but the smart pistol lines need to be an exception to this rule
	// so i am doing this check later and will selectively let draws happen regardless of the convar
	bool debugOverlaysEnabled = Cvar_enable_debug_overlays->GetBool();
	OverlayBase_t** startOverlay = (OverlayBase_t**)((char*)sEngineModule + 0x10DB0968);
	OverlayBase_t* currentOverlay = *startOverlay;
	OverlayBase_t* previousOverlay = nullptr;

	while (currentOverlay != nullptr)
	{
		if (OverlayBase_t__IsDead(currentOverlay))
		{
			// compiler optimisation moment? is EnterCriticalSection not working?
			// basically this would seemingly crash if I didn't do this?
			volatile OverlayBase_t* nextOverlay = currentOverlay->m_pNextOverlay;

			if (previousOverlay != nullptr)
			{
				previousOverlay->m_pNextOverlay = (OverlayBase_t*)nextOverlay;
			}
			else
			{
				*startOverlay = (OverlayBase_t*)nextOverlay;
			}

			OverlayBase_t__DestroyOverlay(currentOverlay);
			currentOverlay = (OverlayBase_t*)nextOverlay;
			continue;
		}

		bool shouldRender;

		if (currentOverlay->m_nCreationTick == -1)
		{
			if ((int)currentOverlay->m_pUnk == -1)
				goto RENDER_OVERLAY;

			// not particularly sure what this is
			shouldRender = (int)currentOverlay->m_pUnk == *((int*)((char*)sEngineModule + 0x10DB0980));
		}
		else
		{
			// not particularly sure what this is
			shouldRender = currentOverlay->m_nCreationTick == *((int*)((char*)sEngineModule + 0x10DB0984));
		}

		if (shouldRender)
		{
		RENDER_OVERLAY:
			// smart pistol's trace lines for some reason use OVERLAY_TRIANGLE not sure why, perhaps the enum is wrong?
			// nothing else that i've found uses it so I'm going to assume its fine to allow that draw type through enable_debug_overlays
			if (bRender && (debugOverlaysEnabled || currentOverlay->m_Type == OVERLAY_TRIANGLE))
				DrawOverlay(currentOverlay);
		}

	NEXT_OVERLAY:
		previousOverlay = currentOverlay;
		currentOverlay = currentOverlay->m_pNextOverlay;
	}

	LeaveCriticalSection((LPCRITICAL_SECTION)((char*)sEngineModule + 0x10DB0A38));
}

ON_DLL_LOAD_CLIENT_RELIESON("engine.dll", DebugOverlay, ConVar, (CModule module))
{
	AUTOHOOK_DISPATCH()

	RenderLine = module.Offset(0x192A70).RCast<RenderLineType>();
	RenderBox = module.Offset(0x192520).RCast<RenderBoxType>();
	RenderWireframeBox = module.Offset(0x193DA0).RCast<RenderBoxType>();

	OverlayBase_t__IsDead = module.Offset(0xACAC0).RCast<OverlayBase_t__IsDeadType>();
	OverlayBase_t__DestroyOverlay = module.Offset(0xAB680).RCast<OverlayBase_t__DestroyOverlayType>();

	sEngineModule = reinterpret_cast<HMODULE>(module.m_nAddress);

	// not in g_pCVar->FindVar by this point for whatever reason, so have to get from memory
	Cvar_enable_debug_overlays = module.Offset(0x10DB0990).RCast<ConVar*>();
	Cvar_enable_debug_overlays->SetValue(false);
	Cvar_enable_debug_overlays->m_pszDefaultValue = (char*)"0";
	Cvar_enable_debug_overlays->AddFlags(FCVAR_CHEAT);
}
