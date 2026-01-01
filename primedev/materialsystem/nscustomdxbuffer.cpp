#include <atomic>
#include <d3d11.h>
#include <map>
#include <mutex>
#include <winternl.h>

ID3D11DeviceContext** DeviceContext;
ID3D11Device** D3D11Device_14E8DD0;

struct Ns_Constant_Buffer
{
	float data[320];
};

AUTOHOOK_INIT()

static Ns_Constant_Buffer NSCustomDXBuffer;
static std::mutex NSCustomDXBufferMutex;

// map to later on associate guid > buffer
static std::map<uint64_t, Ns_Constant_Buffer> NSCustomBuffersPerMaterial = {};

AUTOHOOK(SUB_511D0, materialsystem_dx11.dll + 0x511D0, __int64, __fastcall, (__int64 a1, __int64 a2, __int64 a3, __int64 a4))
{
	int64_t subResult = SUB_511D0(a1, a2, a3, a4);

	uint32_t glueFlags = *(uint32_t*)((uint8_t*)a4 + 176);

	// bind custom buffer when flag is 0x04089901 in rpak mat
	if ((glueFlags & 0x04089901) == 0x04089901)
	{
		if (true)
		{

			D3D11_BUFFER_DESC desc {};
			desc.ByteWidth = sizeof(Ns_Constant_Buffer);
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.StructureByteStride = 0;
			desc.MiscFlags = 0;

			static ID3D11Buffer* resource = nullptr;
			if (!resource)
			{
				if (HRESULT res = (*D3D11Device_14E8DD0)->CreateBuffer(&desc, 0, &resource); !SUCCEEDED(res))
				{
					spdlog::error("Failed to create buffer {:X}", (uint32_t)res);
					return subResult;
				}
			}

			D3D11_MAPPED_SUBRESOURCE mappedSubResource;
			if (!SUCCEEDED((*DeviceContext)->Map(resource, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource)))
			{
				spdlog::error("failed to map data");
				return subResult;
			}

			auto pData = (Ns_Constant_Buffer*)mappedSubResource.pData;

			std::lock_guard<std::mutex> lock(NSCustomDXBufferMutex);

			memcpy(pData, &NSCustomBuffersPerMaterial[*(uint64_t*)a4], sizeof(Ns_Constant_Buffer));

			(*DeviceContext)->Unmap(resource, 0);
			(*DeviceContext)->PSSetConstantBuffers(4, 1, &resource);
		}
	}
	return subResult;
}

template <ScriptContext context> SQRESULT NSSetCustomDXBuffer(HSQUIRRELVM sqvm)
{

	// get the guid as a string to later conv
	auto rPakMaterialGUIDString = (g_pSquirrel[ScriptContext::CLIENT]->getstring(sqvm, 1));

	std::vector<float> NSCustomBufferVector;
	SQArray* NSCustomBufferDataArray = sqvm->_stackOfCurrentFunction[2]._VAL.asArray;

	for (int vIdx = 0; vIdx < NSCustomBufferDataArray->_usedSlots; ++vIdx)
	{
		if (NSCustomBufferDataArray->_values[vIdx]._Type == OT_FLOAT)
		{
			NSCustomBufferVector.push_back(NSCustomBufferDataArray->_values[vIdx]._VAL.asFloat);
		}
	}
	// convert guid to proper usable int
	uint64_t rPakMaterialGUID = std::stoull(rPakMaterialGUIDString, nullptr, 16);

	std::lock_guard<std::mutex> lock(NSCustomDXBufferMutex);

	if ((NSCustomBufferDataArray->_usedSlots) * 4 > sizeof(Ns_Constant_Buffer))
	{
		g_pSquirrel[ScriptContext::SERVER]->raiseerror(
			sqvm,
			fmt::format(
				"Size of Squirrel array exceeds NSCustomDXBuffer size\n\nSquirrel "
				"array in bytes: {}\nNSCustomDXBuffer in bytes: {}",
				(NSCustomBufferDataArray->_usedSlots) * 4,
				sizeof(Ns_Constant_Buffer))
				.c_str());
		return SQRESULT_ERROR;
	}
	for (int vIdx = 0; vIdx < NSCustomBufferVector.size(); ++vIdx)
	{
		// assign buffer to the map using the guid as key
		NSCustomBuffersPerMaterial[rPakMaterialGUID].data[vIdx] = NSCustomBufferVector.at(vIdx);
	}
	return SQRESULT_NULL;
}

ON_DLL_LOAD_CLIENT("materialsystem_dx11.dll", SUB_511D0, (CModule module))
{
	AUTOHOOK_DISPATCH_MODULE(materialsystem_dx11.dll)

	DeviceContext = module.Offset(0x14E8DD8).RCast<ID3D11DeviceContext**>();
	D3D11Device_14E8DD0 = module.Offset(0x14E8DD0).RCast<ID3D11Device**>();
	g_pSquirrel[ScriptContext::CLIENT]->AddFuncRegistration(
		"void",
		"NSSetCustomDXBuffer",
		"string rPakMaterialGUID array NSCustomBufferPerMaterialData",
		"",
		NSSetCustomDXBuffer<ScriptContext::CLIENT>);
}
