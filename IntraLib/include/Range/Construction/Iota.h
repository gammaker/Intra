﻿#pragma once

#include "Range/Concepts.h"
#include "Take.h"

namespace Intra { namespace Range {

template<typename T, typename S> struct RIota
{
	enum: bool {RangeIsInfinite = true};

	T Begin;
	S Step;

	forceinline RIota(null_t=null): Begin(0), Step(0) {}
	forceinline RIota(T begin, S step): Begin(begin), Step(step) {}

	forceinline T First() const {return Begin;}
	forceinline void PopFirst() {Begin = T(Begin+Step);}
	forceinline bool Empty() const {return Step==0;}
	forceinline T operator[](size_t index) const {return Begin+Step*index;}

	forceinline bool operator==(const RIota<T, S>& rhs) const
	{return (Begin==rhs.Begin || Step==0) && Step==rhs.Step;}

	forceinline RTake<RIota> operator()(size_t start, size_t end) const
	{
		INTRA_ASSERT(start<=end);
		return RTake<RIota>(RIota(Begin+Step*start, Step), end-start);
	}
};

template<typename T, typename S=int> forceinline
RIota<T, S> Iota(T begin, S step=1)
{return {begin, step};}

template<typename T, typename S> forceinline
RTake<RIota<T, S>> Iota(T begin, T end, S step)
{return Take(Iota(begin, step), size_t((end-begin+step-1)/step));}


static_assert(IsInputRange<RIota<int, int>>::_, "Not input range???");
static_assert(IsForwardRange<decltype(Iota(1, 2, 3))>::_, "Not forward range???");
static_assert(!IsBidirectionalRange<RIota<int, int>>::_, "Is bidirectional range???");
static_assert(IsRandomAccessRange<RIota<int, int>>::_, "Not random access range???");
static_assert(!IsFiniteRandomAccessRange<RIota<int, int>>::_, "Is finite random access range???");

}}
