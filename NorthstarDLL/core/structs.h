#pragma once
//clang-format off
// About this file:
// This file contains several macros used to define reversed structs
// The reason we use these macros is to make it easier to update existing structs
// when new fields are reversed
// This means we dont have to manually add padding, and recalculate when updating

// Technical note:
// While functionally, these structs act like a regular struct, they are actually
// defined as unions with anonymous structs in them.
// This means that each field is essentially an offset into a union.
// We acknowledge that this goes against C++'s active-member guideline for unions
// However, this is not such a big deal here as these structs will not be constructed

// Usage:
// To use these macros, define a struct like so:
/*
OFFSET_STRUCT(Name)
{
	STRUCT_SIZE(0x100)		// Total in-memory struct size
	FIELD(0x0, int field)	// offset, signature
}
*/

#define OFFSET_STRUCT(name) union name
#define STRUCT_SIZE(size) char __size[size];
#define STRUCT_FIELD_OFFSET(offset, signature)                                                                                             \
	struct                                                                                                                                 \
	{                                                                                                                                      \
		char CONCAT2(pad, __LINE__)[offset];                                                                                               \
		signature;                                                                                                                         \
	};

// Special case for a 0-offset field
#define STRUCT_FIELD_NOOFFSET(offset, signature) signature;

#define FX(f, x) f x

// MSVC does no preprocessing of integer literals.
// On other compilers `0x0` gets processed into `0`
#define TEST_0 ,
#define TEST_0x0 ,

#define ZERO_P_I(a, b, c, ...) a##c
#define IF_ZERO(m) FX(ZERO_P_I, (NIF_, CONCAT2(TEST_, m), 1))

#define NIF_(t, ...) t
#define NIF_1(t, ...) __VA_ARGS__

#define FIELD(offset, signature) IF_ZERO(offset)(STRUCT_FIELD_NOOFFSET, STRUCT_FIELD_OFFSET)(offset, signature)
#define FIELDS FIELD

//clang-format on
