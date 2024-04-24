enum WeaponVarType : __int8
{
	// support for other types isnt really needed
	// and would probably allow for some fucking insane
	// exploit chain somehow
	INTEGER = 1,
	FLOAT32 = 2
};

const int WEAPON_VAR_COUNT = 725;

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
