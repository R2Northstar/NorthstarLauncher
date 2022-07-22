#pragma once

struct __declspec(align(8)) CUtlMemory
{
	void* m_pMemory;
	int64_t m_nAllocationCount;
	int64_t m_nGrowSize;
};

struct __declspec(align(4)) CUtlVector
{
	void* vtable;
	CUtlMemory m_Memory;
	int m_Size;
};

struct CUtlVectorMT
{
	__int64* items;
	__int64 m_nAllocationCount;
	__int64 m_nGrowSize;
	int m_Size;
	int padding_;
	_RTL_CRITICAL_SECTION LOCK;
};

struct CUtlMemoryPool
{
	int m_BlockSize;
	int m_BlocksPerBlob;
	int m_GrowMode;
	int m_BlocksAllocated;
	int unk;
	__int16 m_nAlignment;
	__int16 m_NumBlobs;
	__int64 m_pszAllocOwner;
	__int64 m_Prev_MAYBE;
	__int64 m_Next_MAYBE;
};
