﻿#pragma once

#include "Core/Debug.h"
#include "VirtualMemory.h"
#include "Math/MathEx.h"

namespace Intra { namespace Memory {

struct NewAllocator
{
	static AnyPtr Allocate(size_t& bytes, const SourceInfo& sourceInfo)
	{(void)sourceInfo; return operator new(bytes);}
	
	static void Free(void* ptr, size_t size) {(void)size; operator delete(ptr);}
	
	static size_t GetAlignment() {return sizeof(void*)*2;}
};

struct MallocAllocator
{
	static AnyPtr Allocate(size_t bytes, const SourceInfo& sourceInfo);
	static void Free(void* ptr, size_t size) {(void)size; Free(ptr);}
	static void Free(void* ptr);
	static size_t GetAlignment() {return sizeof(void*)*2;}
};

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
struct SystemHeapAllocator
{
	static AnyPtr Allocate(size_t bytes, const SourceInfo& sourceInfo);
	static void Free(void* ptr, size_t size) {(void)size; Free(ptr);}
	static void Free(void* ptr);
	static size_t GetAlignment() {return sizeof(void*)*2;}
};
#else
typedef MallocAllocator SystemHeapAllocator;
#endif

struct AlignedSystemHeapAllocator
{
	AlignedSystemHeapAllocator(size_t allocatorAlignment=16):
		alignment(Math::Max(allocatorAlignment, sizeof(void*)*2)) {}

	AnyPtr Allocate(size_t& bytes, const SourceInfo& sourceInfo);
	void Free(void* ptr, size_t size);
	size_t GetAlignment() const {return alignment;}

private:
	size_t alignment;
};

struct PageAllocator
{
	static AnyPtr Allocate(size_t& bytes, const SourceInfo& sourceInfo);
	static void Free(void* ptr, size_t size);
	size_t GetAlignment() const;
};

}}
