﻿#pragma once

#include "Concepts/Range.h"
#include "Range/TupleOperation.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH
INTRA_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_COPY_IMPLICITLY_DELETED

template<typename... RANGES> struct RChain;

template<typename R0, typename R1, typename... RANGES> struct RChain<R0, R1, RANGES...>
{
private:
	typedef Meta::CommonTypeRef<Concepts::ReturnValueTypeOf<R0>,
		Concepts::ReturnValueTypeOf<R1>, Concepts::ReturnValueTypeOf<RANGES>...> ReturnValueType;
	typedef RChain<R1, RANGES...> NextChain;
public:
	enum: byte {RangeIsFinite = Concepts::CommonRangeCategoryAllFinite<R0, R1, RANGES...>::Finite};

	forceinline RChain(null_t=null) {}

	forceinline RChain(R0&& r0, R1&& r1, RANGES&&... ranges):
		mRange0(Cpp::Forward<R0>(r0)), mNext(Cpp::Forward<R1>(r1), Cpp::Forward<RANGES>(ranges)...) {}

	forceinline ReturnValueType First() const
	{return mRange0.Empty()? mNext.First(): mRange0.First();}

	forceinline void PopFirst()
	{
		if(!mRange0.Empty()) mRange0.PopFirst();
		else mNext.PopFirst();
	}

	forceinline bool Empty() const
	{return mRange0.Empty() && mNext.Empty();}

	forceinline ReturnValueType Last() const
	{return mNext.Empty()? mRange0.Last(): mNext.Last();}

	forceinline void PopLast()
	{
		if(mNext.Empty()) mRange0.PopLast();
		else mNext.PopLast();
	}


	forceinline size_t Length() const
	{return mRange0.Length()+mNext.Length();}


	forceinline ReturnValueType operator[](size_t index) const
	{
		size_t len = mRange0.Length();
		if(index<len) return mRange0[index];
		return mNext[index-len];
	}

	forceinline RChain operator()(size_t startIndex, size_t endIndex) const
	{
		const size_t len = mRange0.Length();
		return {mRange0(startIndex>len? len: startIndex, len>endIndex? endIndex: len),
			    mNext(startIndex>len? startIndex-len: 0, endIndex>len? endIndex-len: 0)};
	}

	forceinline bool operator==(const RChain& rhs) const
	{return mRange0==rhs.mRange0 && mNext==rhs.mNext;}

private:
	R0 mRange0;
	NextChain mNext;

	forceinline RChain(R0&& r0, NextChain&& ranges):
		mRange0(Cpp::Forward<R0>(r0)), mNext(Cpp::Move(ranges)) {}
};

template<typename R0> struct RChain<R0>
{
private:
	typedef Concepts::ReturnValueTypeOf<R0> ReturnValueType;
public:
	enum: bool {RangeIsFinite = Concepts::IsFiniteRange<R0>::_};

	forceinline RChain(null_t=null) {}

	forceinline RChain(const R0& r0):
		mRange0(r0) {}

	forceinline RChain(R0&& r0):
		mRange0(Cpp::Move(r0)) {}

	forceinline ReturnValueType First() const
	{
		INTRA_DEBUG_ASSERT(!mRange0.Empty());
		return mRange0.First();
	}

	forceinline void PopFirst()
	{
		INTRA_DEBUG_ASSERT(!mRange0.Empty());
		mRange0.PopFirst();
	}

	forceinline bool Empty() const {return mRange0.Empty();}

	forceinline ReturnValueType Last() const {return mRange0.Last();}

	forceinline void PopLast() {mRange0.PopLast();}

	forceinline size_t Length() const {return mRange0.Length();}

	forceinline ReturnValueType operator[](size_t index) const {return mRange0[index];}

	forceinline RChain operator()(size_t startIndex, size_t endIndex) const
	{return RChain(mRange0(startIndex, endIndex));}

	forceinline bool operator==(const RChain& rhs) const
	{return mRange0==rhs.mRange0;}


private:
	R0 mRange0;
};

INTRA_WARNING_POP

template<typename R0, typename... RANGES> forceinline Meta::EnableIf<
	Concepts::IsAsAccessibleRange<R0>::_,
RChain<Concepts::RangeOfTypeNoCRef<R0>, Concepts::RangeOfTypeNoCRef<RANGES>...>> Chain(R0&& range0, RANGES&&... ranges)
{return {Range::Forward<R0>(range0), Range::Forward<RANGES>(ranges)...};}


}}
