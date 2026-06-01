#include "rui.h"

struct gamestate_info_ffa_struct
{
	BYTE gap0[68];
	float endTime;
	const char* statusText;
	float leftTeamScore;
	float rightTeamScore;
	DWORD maxTeamScore;
	const char* factionImage;
	const char* friendlyPlayerCardImage;
	const char* enemyPlayerCardImage;
	BYTE gap78[64];
	DWORD whiteAssetHandle;
	DWORD playerCardImageAssetHandle;
	// struct needs to be 8 byte aligned for these
	uint64_t owordC0[2];
	uint64_t owordD0[2];
	uint64_t topColor[2];
	uint64_t bottomColor[2];
	const char* formattedTimeString;
	const char* gameModeName;
	DWORD otherPlayerCardAssetHandle;
	DWORD leftFillAsset;
	DWORD rightFillAsset;
	float leftTeamScoreDiff;
	float otherTeamScoreDiff;
	DWORD factionImageHandle;
	const char* scoreString;
	const char* rightTeamScoreString;
};

using gamestate_info_ffa_t = void(__fastcall*)(RuiFunctions_t*, RuiGlobals*, RuiInstance*, gamestate_info_ffa_struct*);
gamestate_info_ffa_t o_gamestate_info_ffa = nullptr;

void h_gamestate_info_ffa(RuiFunctions_t* a1, RuiGlobals* a2, RuiInstance* a3, gamestate_info_ffa_struct* a4)
{
	static __m128 xmmword_D3C20 = {25.866, 25.866f, 40.000f, 40.000f};
	static __m128 xmmword_D3C40 = {128.000f, 128.000f, 40.000f, 40.f};
	static __m128 xmmword_D3C50 = {200.000f, 200.000f, 40.000f, 40.000f};
	static __m128 xmmword_D4A00 = {1920.000f, 1920.000f, 1080.000f, 1080.000f};
	static __m128 xmmword_D40E0 = {2.000f, 2.000f, 84.000f, 84.000f};
	static __m128 xmmword_D3CE0 = {48.000f, 48.000f, 48.000f, 48.000f};

	static __m128 enemyColor = {1.000f, 0.188f, 0.014f, 1.f};
	static __m128 friendlyColor = {0.095f, 0.309f, 0.708f, 1.f};

	float endTime = a4->endTime;
	uint64_t globals = (uint64_t)a2;
	float timeLeft = endTime - a2->currentTime;
	char* buffer = (char*)alloca(2048);
	strcpy_s(buffer, 2048, a4->statusText);
	if (strcmp(buffer, "") == 0)
	{
		strcpy_s(buffer, 2048, "#PL_ffa");
	}

	if (timeLeft < 0.0f || endTime == -1.0e30f)
	{
		a4->formattedTimeString = "--:--";
	}
	else if (timeLeft > 30.0f)
	{
		a4->formattedTimeString = a1->printf(a3, "%i:%02i", (unsigned int)((int)timeLeft / 60), (unsigned int)((int)timeLeft % 60));
	}
	else
	{
		a4->formattedTimeString = a1->printf(a3, "%05.2f", timeLeft);
	}

	a4->rightFillAsset = a1->LoadAsset(a3, "rui/hud/gamestate/score_fill_right");
	assetHandle v9 = a1->LoadAsset(a3, "rui/hud/gamestate/score_fill_right");
	bool teamScoreDiff = a4->leftTeamScore < a4->rightTeamScore;
	a4->leftFillAsset = v9;

	__m128 topColor, bottomColor;
	const char* scoreString;
	const char* v23;
	float otherScore;

	if (teamScoreDiff)
	{
		assetHandle enemyPlayerCardImage_1 = a1->LoadAsset(a3, a4->enemyPlayerCardImage);
		const char* friendlyPlayerCardImage = a4->friendlyPlayerCardImage;
		a4->playerCardImageAssetHandle = enemyPlayerCardImage_1;
		assetHandle v26 = a1->LoadAsset(a3, friendlyPlayerCardImage);
		__m128i v27 = _mm_cvtsi32_si128(a4->maxTeamScore);
		a4->otherPlayerCardAssetHandle = v26;
		float v28 = _mm_cvtepi32_ps(v27).m128_f32[0];
		if (v28 == 0.0f)
		{
			return (a1->SetErrorWithReason)(a3, "content\\r2\\ui\\hud\\gamemode_ffa.rui (83,46): divide by zero.\n");
		}

		float rightTeamScore = a4->rightTeamScore;
		a4->leftTeamScoreDiff = rightTeamScore / v28;
		topColor = enemyColor;
		bottomColor = friendlyColor;

		float v30 = a4->leftTeamScore / v28;
		*(__m128*)a4->topColor = topColor;
		*(__m128*)a4->bottomColor = bottomColor;
		a4->otherTeamScoreDiff = v30;
		scoreString = a1->printf(a3, "%.8g", rightTeamScore);
		otherScore = a4->leftTeamScore;
		v23 = "%.8g";
	}
	else
	{
		assetHandle v11 = a1->LoadAsset(a3, a4->friendlyPlayerCardImage);
		const char* enemyPlayerCardImage = a4->enemyPlayerCardImage;
		a4->playerCardImageAssetHandle = v11;
		assetHandle enemyPlayerCardImageAssetHandle_2 = a1->LoadAsset(a3, enemyPlayerCardImage);
		__m128i maxTeamScore = _mm_cvtsi32_si128(a4->maxTeamScore);
		a4->otherPlayerCardAssetHandle = enemyPlayerCardImageAssetHandle_2;
		float maxTeamScore_1 = _mm_cvtepi32_ps(maxTeamScore).m128_f32[0];
		if (maxTeamScore_1 == 0.0f)
		{
			return (a1->SetErrorWithReason)(a3, "content\\r2\\ui\\hud\\gamemode_ffa.rui (83,46): divide by zero.\n");
		}

		float leftTeamScore_1 = a4->leftTeamScore;
		a4->leftTeamScoreDiff = leftTeamScore_1 / maxTeamScore_1;
		topColor = friendlyColor;
		bottomColor = enemyColor;
		float v20 = a4->rightTeamScore / maxTeamScore_1;
		*(__m128*)a4->topColor = topColor;
		*(__m128*)a4->bottomColor = bottomColor;
		a4->otherTeamScoreDiff = v20;
		scoreString = a1->printf(a3, "%.8g", leftTeamScore_1);
		otherScore = a4->rightTeamScore;
		v23 = "%.8g";
	}

	a4->scoreString = scoreString;
	const char* v31 = a1->printf(a3, v23, otherScore);
	*(__m128*)a4->owordD0 = bottomColor;
	*(__m128*)a4->owordC0 = topColor;
	a4->rightTeamScoreString = v31;
	a4->whiteAssetHandle = a1->LoadAsset(a3, "white");
	const char* factionImage = a4->factionImage;
	a4->gameModeName = a1->localize(a3, buffer);
	a4->factionImageHandle = a1->LoadAsset(a3, factionImage);

	__m128* transformSizes = a1->GetTransformSize(a3);
	transformSizes[3] = (__m128)xmmword_D3C50;
	a1->executeTransform(a3, 1);
	transformSizes[4] = (__m128)xmmword_D3C50;
	a1->executeTransform(a3, 2);
	transformSizes[5] = (__m128)xmmword_D4A00;
	transformSizes[6] = (__m128)xmmword_D40E0;

	__m128 v35;
	// v35.m128_u64[1] = 0x4220000042200000ULL;
	v35 = _mm_set_ps(40.0f, 40.0f, 0.0f, 0.0f);
	transformSizes[7] = (__m128)xmmword_D4A00;
	transformSizes[9] = (__m128)xmmword_D3C20;
	transformSizes[8] = (__m128)xmmword_D3C40;
	transformSizes[10] = (__m128)xmmword_D3C20;

	v35 = (a1->unknown_5)(a3, 3LL, 4LL);
	transformSizes[11] = v35;
	v35 = (a1->unknown_5)(a3, 4LL, 5LL);
	transformSizes[12] = v35;
	transformSizes[13] = (a1->GetTextSize)(a3, 42LL);
	transformSizes[14] = (a1->GetTextSize)(a3, 60LL);
	transformSizes[15] = (a1->GetTextSize)(a3, 78LL);
	transformSizes[16] = (a1->GetTextSize)(a3, 348LL);
	transformSizes[17] = (a1->GetTextSize)(a3, 366LL);
	transformSizes[18] = xmmword_D3CE0;

	return (a1->executeTransform)(a3, 0x9ELL);
}

ON_DLL_LOAD("ui(11).dll", Rui, (CModule module))
{
	o_gamestate_info_ffa = module.Offset(0x3E8E0).RCast<gamestate_info_ffa_t>();
	HookAttach(&(PVOID&)o_gamestate_info_ffa, (PVOID)h_gamestate_info_ffa);
}
