#pragma once
enum WeaponVarType : __int8
{
	WVT_INTEGER = 1,
	WVT_FLOAT32 = 2,
	WVT_BOOLEAN = 3,
	WVT_STRING = 4,
	// NOT SUPPORTED ATM //
	WVT_ASSET = 5,
	WVT_VECTOR = 6
};

struct WeaponModCallback
{
	int priority;
	SQObject func;
};

#pragma pack(push, 1)
class WeaponVarInfo
{
public:
	const char unk_0[25]; // 0x0
	char type; // 0x19
	int unk_1A;
	unsigned short offset;
};
#pragma pack(pop)
static_assert(sizeof(WeaponVarInfo) == 0x20);

const int WEAPON_VAR_COUNT = 725;
extern std::map<size_t, std::string> sv_modWeaponVarStrings;
extern std::map<size_t, std::string> cl_modWeaponVarStrings;
extern std::hash<std::string> hasher;
