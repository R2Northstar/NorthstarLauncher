#pragma once
#include "squirreldatatypes.h"

enum SQRESULT : SQInteger
{
	SQRESULT_ERROR = -1,
	SQRESULT_NULL = 0,
	SQRESULT_NOTNULL = 1,
};

typedef SQRESULT (*SQFunction)(HSquirrelVM* sqvm);

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
	{SQRESULT::SQRESULT_ERROR, "SQRESULT_ERROR"},
	{SQRESULT::SQRESULT_NULL, "SQRESULT_NULL"},
	{SQRESULT::SQRESULT_NOTNULL, "SQRESULT_NOTNULL"}};

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
	SQFunction funcPtr;

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

typedef std::vector<std::function<void()>> FunctionVector;

// clang-format off
template <typename T>
concept is_map =
	// Simple maps
	std::same_as<T, std::map<typename T::key_type, typename T::mapped_type, typename T::key_compare, typename T::allocator_type>> ||
	std::same_as<T, std::unordered_map<typename T::key_type, typename T::mapped_type, typename T::hasher, typename T::key_equal, typename T::allocator_type>> ||

	// Nested maps
	std::same_as <
		std::map<typename T::key_type, typename T::mapped_type, typename T::key_compare, typename T::allocator_type>,
		std::map<typename T::key_type, typename T::mapped_type, typename T::key_compare, typename T::allocator_type>
	> ||
	std::same_as <
		std::unordered_map<typename T::key_type, typename T::mapped_type, typename T::key_compare, typename T::allocator_type>,
		std::unordered_map<typename T::key_type, typename T::mapped_type, typename T::key_compare, typename T::allocator_type>
	> ||
	std::same_as <
		std::unordered_map<typename T::key_type, typename T::mapped_type, typename T::key_compare, typename T::allocator_type>,
		std::map<typename T::key_type, typename T::mapped_type, typename T::key_compare, typename T::allocator_type>
	> ||
	std::same_as <
		std::map<typename T::key_type, typename T::mapped_type, typename T::key_compare, typename T::allocator_type>,
		std::unordered_map<typename T::key_type, typename T::mapped_type, typename T::key_compare, typename T::allocator_type>
	>
;

template<typename T>
concept is_iterable = requires(std::ranges::range_value_t<T> x)
{
    x.begin();          // must have `x.begin()`
    x.end();            // and `x.end()`
};

// clang-format on

class SquirrelMessage
{
  public:
	const char* function_name;
	FunctionVector args;
};

class SquirrelMessageBuffer
{

  private:
	std::vector<SquirrelMessage> messages = {};

  public:
	std::mutex mutex;
	std::optional<SquirrelMessage> pop()
	{
		std::lock_guard<std::mutex> guard(mutex);
		if (!messages.empty())
		{
			auto message = messages.back();
			messages.pop_back();
			return message;
		}
		else
		{
			return std::nullopt;
		}
	}

	void push(SquirrelMessage message)
	{
		std::lock_guard<std::mutex> guard(mutex);
		messages.push_back(message);
	}
};

class SquirrelAsset
{
  public:
	std::string path;
	SquirrelAsset(std::string path) : path(path) {};
};
