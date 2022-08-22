#pragma once

// stolen from ttf2sdk: sqvm types
typedef float SQFloat;
typedef long SQInteger;
typedef unsigned long SQUnsignedInteger;
typedef char SQChar;
typedef SQUnsignedInteger SQBool;

enum SQRESULT : SQInteger
{
	SQRESULT_ERROR = -1,
	SQRESULT_NULL = 0,
	SQRESULT_NOTNULL = 1,
};

typedef SQRESULT (*SQFunction)(void* sqvm);

enum class eSQReturnType
{
	Float = 0x1,
	Vector = 0x3,
	Integer = 0x5,
	Boolean = 0x6,
	Entity = 0xD,
	String = 0x21,
	Default = 0x20,
	Arrays = 0x25,
	Asset = 0x28,
	Table = 0x26,
};

const std::map<SQRESULT, const char*> PrintSQRESULT = {
	{SQRESULT_ERROR, "ERROR"},
	{SQRESULT_NULL, "NULL"},
	{SQRESULT_NOTNULL, "NOTNULL"}
};

struct CompileBufferState
{
	const SQChar* buffer;
	const SQChar* bufferPlusLength;
	const SQChar* bufferAgain;

	CompileBufferState(const std::string& code)
	{
		buffer = code.c_str();
		bufferPlusLength = code.c_str() + code.size();
		bufferAgain = code.c_str();
	}
};

struct CallInfo;
struct SQTable;
struct SQString;
struct SQFunctionProto;
struct SQClosure;
struct SQSharedState;
struct StringTable;
struct SQStructInstance;
struct SQStructDef;
struct SQNativeClosure;
struct SQArray;
struct SQInstruction;

/* 127 */
enum SQObjectType : uint32_t
{
	_RT_NULL = 0x1,
	_RT_INTEGER = 0x2,
	_RT_FLOAT = 0x4,
	_RT_BOOL = 0x8,
	_RT_STRING = 0x10,
	_RT_TABLE = 0x20,
	_RT_ARRAY = 0x40,
	_RT_USERDATA = 0x80,
	_RT_CLOSURE = 0x100,
	_RT_NATIVECLOSURE = 0x200,
	_RT_GENERATOR = 0x400,
	OT_USERPOINTER = 0x800,
	_RT_USERPOINTER = 0x800,
	_RT_THREAD = 0x1000,
	_RT_FUNCPROTO = 0x2000,
	_RT_CLASS = 0x4000,
	_RT_INSTANCE = 0x8000,
	_RT_WEAKREF = 0x10000,
	OT_VECTOR = 0x40000,
	SQOBJECT_CANBEFALSE = 0x1000000,
	OT_NULL = 0x1000001,
	OT_BOOL = 0x1000008,
	SQOBJECT_DELEGABLE = 0x2000000,
	SQOBJECT_NUMERIC = 0x4000000,
	OT_INTEGER = 0x5000002,
	OT_FLOAT = 0x5000004,
	SQOBJECT_REF_COUNTED = 0x8000000,
	OT_STRING = 0x8000010,
	OT_ARRAY = 0x8000040,
	OT_CLOSURE = 0x8000100,
	OT_NATIVECLOSURE = 0x8000200,
	OT_ASSET = 0x8000400,
	OT_THREAD = 0x8001000,
	OT_FUNCPROTO = 0x8002000,
	OT_CLAAS = 0x8004000,
	OT_STRUCT = 0x8200000,
	OT_WEAKREF = 0x8010000,
	OT_TABLE = 0xA000020,
	OT_USERDATA = 0xA000080,
	OT_INSTANCE = 0xA008000,
	OT_ENTITY = 0xA400000,
};

/* 156 */
union alignas(8) SQObjectValue
{
	SQString* asString;
	SQTable* asTable;
	SQClosure* asClosure;
	SQFunctionProto* asFuncProto;
	SQStructDef* asStructDef;
	uint64_t asInteger;
	SQStructInstance* asStructInstance;
	float asFloat;
	SQNativeClosure* asNativeClosure;
	SQArray* asArray;
};

/* 128 */
struct alignas(8) SQObject
{
	SQObjectType _Type;
	uint32_t _structOffset;
	SQObjectValue _VAL;
};

struct tableNode
{
	SQObject val;
	SQObject key;
	tableNode* next;
};

/* 138 */
struct alignas(8) SQString
{
	void* vftable;
	uint32_t uiRef;
	uint32_t uiRef1;
	SQString* _next_maybe;
	SQSharedState* sharedState;
	uint32_t length;
	uint8_t gap_24[4];
	char _hash[8];
	char _val[1];
};

/* 137 */
struct alignas(8) SQTable
{
	void* vftable;
	uint8_t gap_08[4];
	uint32_t uiRef;
	uint8_t gap_10[8];
	void* pointer_18;
	void* pointer_20;
	void* _sharedState;
	uint64_t field_30;
	tableNode* _nodes;
	uint32_t _numOfNodes;
	uint32_t size;
	uint32_t field_48;
	uint32_t _usedNodes;
	uint8_t _gap_50[20];
	uint32_t field_64;
	uint8_t _gap_68[80];
};

/* 140 */
struct alignas(8) SQClosure
{
	void* vftable;
	uint8_t gap_08[4];
	uint32_t uiRef;
	void* pointer_10;
	void* pointer_18;
	void* pointer_20;
	void* sharedState;
	SQObject obj_30;
	SQObject _function;
	SQObject* _outervalues;
	uint8_t gap_58[8];
	uint8_t gap_60[96];
	SQObject* objectPointer_C0;
};

/* 139 */
struct alignas(8) SQFunctionProto
{
	void* vftable;
	uint8_t gap_08[4];
	uint32_t uiRef;
	uint8_t gap_10[8];
	void* pointer_18;
	void* pointer_20;
	void* sharedState;
	void* pointer_30;
	SQObject fileName;
	SQObject funcName;
	SQObject obj_58;
	uint8_t gap_68[64];
	uint32_t nParameters;
	uint8_t gap_AC[60];
	uint32_t nDefaultParams;
	uint8_t gap_EC[200];
};

/* 152 */
struct SQStructDef
{
	uint8_t gap_0[56];
	SQString* name;
	uint8_t gap_[300];
};

/* 150 */
struct SQStructInstance
{
	void* vftable;
	uint8_t gap_8[16];
	void* pointer_18;
	uint8_t gap_20[8];
	SQSharedState* _sharedState;
	uint8_t gap_30[8];
	SQObject data[1];
};

/* 157 */
struct alignas(8) SQNativeClosure
{
	void* vftable;
	uint8_t gap_08[4];
	uint32_t uiRef;
	uint8_t gap_10[88];
	SQString* _name;
	uint8_t gap_0[300];
};

/* 148 */
struct SQSharedState
{
	uint8_t gap_0[72];
	StringTable* _stringtable;
	uint8_t gap_50[30000];
};

/* 149 */
struct StringTable
{
	uint8_t gap_0[12];
	int _numofslots;
	uint8_t gap_10[200];
};

/* 129 */
struct alignas(8) HSquirrelVM
{
	void* vftable;
	uint32_t uiRef;
	uint8_t gap_8[12];
	void* _toString;
	void* _roottable_pointer;
	void* pointer_28;
	CallInfo* ci;
	CallInfo* _callsstack;
	uint32_t _callsstacksize;
	uint32_t _stackbase;
	SQObject* _stackOfCurrentFunction;
	SQSharedState* sharedState;
	void* pointer_58;
	void* pointer_60;
	uint32_t _top;
	SQObject* _stack;
	uint8_t gap_78[8];
	SQObject* _vargsstack;
	uint8_t gap_88[8];
	SQObject temp_reg;
	uint8_t gapA0[8];
	void* pointer_A8;
	uint8_t gap_B0[8];
	SQObject _roottable_object;
	SQObject _lasterror;
	SQObject _errorHandler;
	uint64_t field_E8;
	uint32_t traps;
	uint8_t gap_F4[12];
	uint32_t _nnativecalls;
	uint32_t _suspended;
	uint32_t _suspended_root;
	uint32_t _callstacksize;
	uint32_t _suspended_target;
	uint32_t field_114;
	uint32_t _suspend_varargs;
	SQObject* _object_pointer_120;
};

/* 136 */
struct alignas(8) CallInfo
{
	SQInstruction* ip;
	SQObject* _literals;
	SQObject obj10;
	SQObject closure;
	uint32_t _etraps[4];
	uint32_t _root;
	short _vargs_size;
	short _vargs_base;
};

/* 135 */
enum SQOpcode : int
{
	_OP_LOAD = 0x0,
	_OP_LOADCOPY = 0x1,
	_OP_LOADINT = 0x2,
	_OP_LOADFLOAT = 0x3,
	_OP_DLOAD = 0x4,
	_OP_TAILCALL = 0x5,
	_OP_CALL = 0x6,
	_OP_PREPCALL = 0x7,
	_OP_PREPCALLK = 0x8,
	_OP_GETK = 0x9,
	_OP_MOVE = 0xA,
	_OP_NEWSLOT = 0xB,
	_OP_DELETE = 0xC,
	_OP_SET = 0xD,
	_OP_GET = 0xE,
	_OP_EQ = 0xF,
	_OP_NE = 0x10,
	_OP_ARITH = 0x11,
	_OP_BITW = 0x12,
	_OP_RETURN = 0x13,
	_OP_LOADNULLS = 0x14,
	_OP_LOADROOTTABLE = 0x15,
	_OP_LOADBOOL = 0x16,
	_OP_DMOVE = 0x17,
	_OP_JMP = 0x18,
	_OP_JNZ = 0x19,
	_OP_JZ = 0x1A,
	_OP_LOADFREEVAR = 0x1B,
	_OP_VARGC = 0x1C,
	_OP_GETVARGV = 0x1D,
	_OP_NEWTABLE = 0x1E,
	_OP_NEWARRAY = 0x1F,
	_OP_APPENDARRAY = 0x20,
	_OP_GETPARENT = 0x21,
	_OP_COMPOUND_ARITH = 0x22,
	_OP_COMPOUND_ARITH_LOCAL = 0x23,
	_OP_INCREMENT_PREFIX = 0x24,
	_OP_INCREMENT_PREFIX_LOCAL = 0x25,
	_OP_INCREMENT_PREFIX_STRUCTFIELD = 0x26,
	_OP_INCREMENT_POSTFIX = 0x27,
	_OP_INCREMENT_POSTFIX_LOCAL = 0x28,
	_OP_INCREMENT_POSTFIX_STRUCTFIELD = 0x29,
	_OP_CMP = 0x2A,
	_OP_EXISTS = 0x2B,
	_OP_INSTANCEOF = 0x2C,
	_OP_NEG = 0x2D,
	_OP_NOT = 0x2E,
	_OP_BWNOT = 0x2F,
	_OP_CLOSURE = 0x30,
	_OP_FOREACH = 0x31,
	_OP_FOREACH_STATICARRAY_START = 0x32,
	_OP_FOREACH_STATICARRAY_NEXT = 0x33,
	_OP_FOREACH_STATICARRAY_NESTEDSTRUCT_START = 0x34,
	_OP_FOREACH_STATICARRAY_NESTEDSTRUCT_NEXT = 0x35,
	_OP_DELEGATE = 0x36,
	_OP_CLONE = 0x37,
	_OP_TYPEOF = 0x38,
	_OP_PUSHTRAP = 0x39,
	_OP_POPTRAP = 0x3A,
	_OP_THROW = 0x3B,
	_OP_CLASS = 0x3C,
	_OP_NEWSLOTA = 0x3D,
	_OP_EQ_LITERAL = 0x3E,
	_OP_NE_LITERAL = 0x3F,
	_OP_FOREACH_SETUP = 0x40,
	_OP_ASSERT_FAILED = 0x41,
	_OP_ADD = 0x42,
	_OP_SUB = 0x43,
	_OP_MUL = 0x44,
	_OP_DIV = 0x45,
	_OP_MOD = 0x46,
	_OP_PREPCALLK_CALL = 0x47,
	_OP_PREPCALLK_MOVE_CALL = 0x48,
	_OP_PREPCALLK_LOADINT_CALL = 0x49,
	_OP_CMP_JZ = 0x4A,
	_OP_INCREMENT_LOCAL_DISCARD_JMP = 0x4B,
	_OP_JZ_RETURN = 0x4C,
	_OP_JZ_LOADBOOL_RETURN = 0x4D,
	_OP_NEWVECTOR = 0x4E,
	_OP_ZEROVECTOR = 0x4F,
	_OP_GET_VECTOR_COMPONENT = 0x50,
	_OP_SET_VECTOR_COMPONENT = 0x51,
	_OP_VECTOR_COMPONENT_MINUSEQ = 0x52,
	_OP_VECTOR_COMPONENT_PLUSEQ = 0x53,
	_OP_VECTOR_COMPONENT_MULEQ = 0x54,
	_OP_VECTOR_COMPONENT_DIVEQ = 0x55,
	_OP_VECTOR_NORMALIZE = 0x56,
	_OP_VECTOR_NORMALIZE_IN_PLACE = 0x57,
	_OP_VECTOR_DOT_PRODUCT = 0x58,
	_OP_VECTOR_DOT_PRODUCT2D = 0x59,
	_OP_VECTOR_CROSS_PRODUCT = 0x5A,
	_OP_VECTOR_CROSS_PRODUCT2D = 0x5B,
	_OP_VECTOR_LENGTH = 0x5C,
	_OP_VECTOR_LENGTHSQR = 0x5D,
	_OP_VECTOR_LENGTH2D = 0x5E,
	_OP_VECTOR_LENGTH2DSQR = 0x5F,
	_OP_VECTOR_DISTANCE = 0x60,
	_OP_VECTOR_DISTANCESQR = 0x61,
	_OP_VECTOR_DISTANCE2D = 0x62,
	_OP_VECTOR_DISTANCE2DSQR = 0x63,
	_OP_INCREMENT_LOCAL_DISCARD = 0x64,
	_OP_FASTCALL = 0x65,
	_OP_FASTCALL_NATIVE = 0x66,
	_OP_FASTCALL_NATIVE_ARGTYPECHECK = 0x67,
	_OP_FASTCALL_ENV = 0x68,
	_OP_FASTCALL_NATIVE_ENV = 0x69,
	_OP_FASTCALL_NATIVE_ENV_ARGTYPECHECK = 0x6A,
	_OP_LOADGLOBALARRAY = 0x6B,
	_OP_GETGLOBAL = 0x6C,
	_OP_SETGLOBAL = 0x6D,
	_OP_COMPOUND_ARITH_GLOBAL = 0x6E,
	_OP_GETSTRUCTFIELD = 0x6F,
	_OP_SETSTRUCTFIELD = 0x70,
	_OP_COMPOUND_ARITH_STRUCTFIELD = 0x71,
	_OP_NEWSTRUCT = 0x72,
	_OP_GETSUBSTRUCT = 0x73,
	_OP_GETSUBSTRUCT_DYNAMIC = 0x74,
	_OP_TYPECAST = 0x75,
	_OP_TYPECHECK = 0x76,
	_OP_TYPECHECK_ORNULL = 0x77,
	_OP_TYPECHECK_NOTNULL = 0x78,
	_OP_CHECK_ENTITY_CLASS = 0x79,
	_OP_UNREACHABLE = 0x7A,
	_OP_ARRAY_RESIZE = 0x7B,
};

/* 141 */
struct alignas(8) SQStackInfos
{
	char* _name;
	char* _sourceName;
	uint32_t _line;
};

/* 151 */
struct alignas(4) SQInstruction
{
	int op;
	int arg1;
	int output;
	uint16_t arg2;
	uint16_t arg3;
};

/* 154 */
struct SQLexer
{
	uint8_t gap_0[112];
};

/* 153 */
struct SQCompiler
{
	uint8_t gap_0[4];
	uint32_t _token;
	uint8_t gap_8[8];
	SQObject object_10;
	SQLexer lexer;
	uint8_t gap_1[768];
};

/* 155 */
struct CSquirrelVM
{
	uint8_t gap_0[8];
	HSquirrelVM* sqvm;
};

struct SQVector
{
	SQObjectType _Type;
	float x;
	float y;
	float z;
};

struct SQArray
{
	void* vftable;
	uint32_t uiRef;
	uint8_t gap_24[36];
	SQObject* _values;
	uint32_t _usedSlots;
	uint32_t _allocated;
};

#define SQOBJ_INCREMENT_REFERENCECOUNT(val)                                                                                                      \
	if ((val->_Type & SQOBJECT_REF_COUNTED) != 0)                                                                                          \
		++val->_VAL.asString->uiRef;

#define SQOBJ_DECREMENT_REFERENCECOUNT(val)                                                                                                      \
	if ((val->_Type & SQOBJECT_REF_COUNTED) != 0)                                                                                          \
	{                                                                                                                                      \
		if (val->_VAL.asString->uiRef-- == 1)                                                                                              \
		{                                                                                                                                  \
			spdlog::info("Deleted SQObject of type {} with address {:X}", sq_getTypeName(val->_Type), val->_VAL.asInteger);                \
			(*(void(__fastcall**)(SQString*))(&val->_VAL.asString->vftable[1]))(val->_VAL.asString);                                       \
		}                                                                                                                                  \
	}

struct SQFuncRegistration
{
	const char* squirrelFuncName;
	const char* cppFuncName;
	const char* helpText;
	const char* returnTypeString;
	const char* argTypes;
	uint32_t unknown1;
	uint32_t devLevel;
	const char* shortNameMaybe;
	uint32_t unknown2;
	eSQReturnType returnType;
	uint32_t* externalBufferPointer;
	uint64_t externalBufferSize;
	uint64_t unknown3;
	uint64_t unknown4;
	void* funcPtr;

	SQFuncRegistration()
	{
		memset(this, 0, sizeof(SQFuncRegistration));
		this->returnType = eSQReturnType::Default;
	}
};

enum class ScriptContext : int
{
	SERVER,
	CLIENT,
	UI,
};

const char* GetContextName(ScriptContext context);
eSQReturnType SQReturnTypeFromString(const char* pReturnType);
const char* SQTypeNameFromID(const int iTypeId);

// core sqvm funcs
typedef int64_t (*RegisterSquirrelFuncType)(void* sqvm, SQFuncRegistration* funcReg, char unknown);
typedef void (*sq_defconstType)(void* sqvm, const SQChar* name, int value);

typedef SQRESULT (*sq_compilebufferType)(void* sqvm, CompileBufferState* compileBuffer, const char* file, int a1, SQBool bShouldThrowError);
typedef SQRESULT (*sq_callType)(void* sqvm, SQInteger iArgs, SQBool bShouldReturn, SQBool bThrowError);
typedef SQInteger (*sq_raiseerrorType)(void* sqvm, const SQChar* pError);

// sq stack array funcs
typedef void (*sq_newarrayType)(void* sqvm, SQInteger iStackpos);
typedef SQRESULT (*sq_arrayappendType)(void* sqvm, SQInteger iStackpos);

// sq table funcs
typedef SQRESULT (*sq_newtableType)(void* sqvm);
typedef SQRESULT (*sq_newslotType)(void* sqvm, SQInteger idx, SQBool bStatic);

// sq stack push funcs
typedef void (*sq_pushroottableType)(void* sqvm);
typedef void (*sq_pushstringType)(void* sqvm, const SQChar* pStr, SQInteger iLength);
typedef void (*sq_pushintegerType)(void* sqvm, SQInteger i);
typedef void (*sq_pushfloatType)(void* sqvm, SQFloat f);
typedef void (*sq_pushboolType)(void* sqvm, SQBool b);
typedef void (*sq_pushAssetType)(void* sqvm, const SQChar* pName, SQInteger iLength);

// sq stack get funcs
typedef const SQChar* (*sq_getstringType)(void* sqvm, SQInteger stackpos);
typedef SQInteger (*sq_getintegerType)(void* sqvm, SQInteger stackpos);
typedef SQFloat (*sq_getfloatType)(void*, SQInteger stackpos);
typedef SQBool (*sq_getboolType)(void*, SQInteger stackpos);
typedef SQRESULT (*sq_getType)(void* sqvm, SQInteger stackpos);

template <ScriptContext context> class SquirrelManager
{
  private:
	std::vector<SQFuncRegistration*> m_funcRegistrations;

  public:
	void* sqvm;
	void* sqvm2;

	bool m_bCompilationErrorsFatal = false;

	#pragma region SQVM funcs
	RegisterSquirrelFuncType RegisterSquirrelFunc;
	sq_defconstType __sq_defconst;

	sq_compilebufferType __sq_compilebuffer;
	sq_callType __sq_call;
	sq_raiseerrorType __sq_raiseerror;

	sq_newarrayType __sq_newarray;
	sq_arrayappendType __sq_arrayappend;

	sq_newtableType __sq_newtable;
	sq_newslotType __sq_newslot;

	sq_pushroottableType __sq_pushroottable;
	sq_pushstringType __sq_pushstring;
	sq_pushintegerType __sq_pushinteger;
	sq_pushfloatType __sq_pushfloat;
	sq_pushboolType __sq_pushbool;
	sq_pushAssetType __sq_pushasset;

	sq_getstringType __sq_getstring;
	sq_getintegerType __sq_getinteger;
	sq_getfloatType __sq_getfloat;
	sq_getboolType __sq_getbool;
	sq_getType __sq_get;
	#pragma endregion

  public:
	SquirrelManager() : sqvm(nullptr) {}

	void VMCreated(void* newSqvm);
	void VMDestroyed();
	void ExecuteCode(const char* code);
	void AddFuncRegistration(std::string returnType, std::string name, std::string argTypes, std::string helpText, SQFunction func);
	SQRESULT setupfunc(const SQChar* funcname);

	#pragma region SQVM func wrappers
	inline void defconst(void* sqvm, const SQChar* pName, int nValue)
	{
		__sq_defconst(sqvm, pName, nValue);
	}

	inline SQRESULT compilebuffer(CompileBufferState* bufferState, const SQChar* bufferName = "unnamedbuffer", const SQBool bShouldThrowError = false)
	{
		return __sq_compilebuffer(sqvm2, bufferState, bufferName, -1, bShouldThrowError);
	}

	inline SQRESULT call(void* sqvm, const SQInteger args)
	{
		return __sq_call(sqvm, args + 1, false, false);
	}

	inline SQInteger raiseerror(void* sqvm, const const SQChar* sError)
	{
		return __sq_raiseerror(sqvm, sError);
	}

	inline void newarray(void* sqvm, const SQInteger stackpos = 0) 
	{
		__sq_newarray(sqvm, stackpos);
	}

	inline SQRESULT arrayappend(void* sqvm, const SQInteger stackpos) 
	{
		return __sq_arrayappend(sqvm, stackpos);
	}

	inline SQRESULT newtable(void* sqvm) 
	{
		return __sq_newtable(sqvm);
	}

	inline SQRESULT newslot(void* sqvm, SQInteger idx, SQBool bStatic)
	{
		return __sq_newslot(sqvm, idx, bStatic);
	}

	inline void pushroottable(void* sqvm)
	{
		__sq_pushroottable(sqvm);
	}

	inline void pushstring(void* sqvm, const SQChar* sVal, int length = -1)
	{
		__sq_pushstring(sqvm, sVal, length);
	}

	inline void pushinteger(void* sqvm, const SQInteger iVal)
	{
		__sq_pushinteger(sqvm, iVal);
	}

	inline void pushfloat(void* sqvm, const SQFloat flVal)
	{
		__sq_pushfloat(sqvm, flVal);
	}

	inline void pushbool(void* sqvm, const SQBool bVal)
	{
		__sq_pushbool(sqvm, bVal);
	}

	inline void pushasset(void* sqvm, const SQChar* sVal, int length = -1)
	{
		__sq_pushasset(sqvm, sVal, length);
	}

	inline const SQChar* getstring(void* sqvm, const SQInteger stackpos)
	{
		return __sq_getstring(sqvm, stackpos);
	}

	inline SQInteger getinteger(void* sqvm, const SQInteger stackpos)
	{
		return __sq_getinteger(sqvm, stackpos);
	}

	inline SQFloat getfloat(void* sqvm, const SQInteger stackpos)
	{
		return __sq_getfloat(sqvm, stackpos);
	}

	inline SQBool getbool(void* sqvm, const SQInteger stackpos)
	{
		return __sq_getbool(sqvm, stackpos);
	}

	inline SQRESULT get(void* sqvm, const SQInteger stackpos)
	{
		return __sq_get(sqvm, stackpos);
	}
	#pragma endregion
};

template <ScriptContext context> SquirrelManager<context>* g_pSquirrel;
