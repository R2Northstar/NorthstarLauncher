#include <winternl.h>
#include <d3d11.h>

AUTOHOOK_INIT()

struct DynamicTexture
{
	int64_t qword_0[2];
	ID3D11Resource* texture;
	ID3D11ShaderResourceView* resourceView;
	BYTE gap_20[8];
	int64_t qword_28;
	ID3D11ShaderResourceView* resourceView2;
	BYTE gap_[30];
	uint16_t samplerStateId;
	WORD word_58;
	WORD word_5A;
	BYTE gap_5C[4];
};

int16_t word_21CB88;
ID3D11DeviceContext** DeviceContext;
DynamicTexture*** Src;

int Swap_Texture = 15;

void ConCommand_ChangeDynamicTexture(const CCommand& args)
{
	if (args.ArgC() < 2)
	{
		spdlog::warn("Usage: ChangeDynamicTexture <texture_id>");
		return;
	}
	int textureId = std::atoi(args.Arg(1));
	Swap_Texture = textureId;
	spdlog::info("Swap_Texture set to: {}", Swap_Texture);

}

AUTOHOOK(SUB_25B90, materialsystem_dx11.dll + 0x25B90, void, __fastcall, ())
{
	(*DeviceContext)->PSSetShaderResources(20LL, 1LL, &(*Src)[0][Swap_Texture].resourceView);
	(*DeviceContext)->PSSetShaderResources(21LL, 1LL, &(*Src)[0][41].resourceView);
}

ON_DLL_LOAD_CLIENT("materialsystem_dx11.dll", SUB_25B90, (CModule module))
{
	RegisterConCommand("changedynamictexture", ConCommand_ChangeDynamicTexture, "changes dynamic texture", FCVAR_GAMEDLL);

	word_21CB88 = *module.Offset(0x21CB88).RCast<int16_t*>();
	DeviceContext = module.Offset(0x14E8DD8).RCast<ID3D11DeviceContext**>();
	Src = module.Offset(0x19B13D8).RCast<DynamicTexture***>();

	AUTOHOOK_DISPATCH()
}
