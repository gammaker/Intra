#pragma once

#include "Meta/Operators.h"
#include "Range/ForwardDecls.h"
#include "Range/Concepts.h"
#include "Utils/Optional.h"
#include "Range/Construction/Take.h"
#include "Range/Construction/TakeUntil.h"

namespace Intra { namespace Range {

INTRA_WARNING_PUSH_DISABLE_COPY_MOVE_IMPLICITLY_DELETED

template<typename R, typename P1, typename P2> struct RSplit
{
	enum: bool {RangeIsFinite = IsFiniteRange<R>::_, RangeIsInfinite = IsInfiniteRange<R>::_};

	forceinline RSplit(null_t=null):
		mOriginalRange(null), mIsSkippedDelimiter(), mIsElementDelimiter() {}

	forceinline RSplit(R&& range, P1 isSkippedDelimiter, P2 isElementDelimiter):
		mOriginalRange(Meta::Move(range)), mIsSkippedDelimiter(isSkippedDelimiter),
		mIsElementDelimiter(isElementDelimiter) {PopFirst();}

	forceinline RSplit(const R& range, P1 isSkippedDelimiter, P2 isElementDelimiter):
		mOriginalRange(range), mIsSkippedDelimiter(isSkippedDelimiter),
		mIsElementDelimiter(isElementDelimiter) {PopFirst();}

	forceinline bool Empty() const {return mFirst.Empty();}

	void PopFirst()
	{
		if(mIsElementDelimiter!=null && mIsElementDelimiter()(mOriginalRange.First()))
		{
			mFirst = Take(mOriginalRange, 1);
			mOriginalRange.PopFirst();
			return;
		}
		mFirst = Range::TakeUntil(mOriginalRange, [this](const ValueTypeOf<R>& v)
		{
			return (mIsSkippedDelimiter!=null && mIsSkippedDelimiter()(v)) ||
				(mIsElementDelimiter!=null && mIsElementDelimiter()(v));
		});
	}

	forceinline ResultOfTake<R> First() const {INTRA_ASSERT(!Empty()); return mFirst;}

	bool operator==(const RSplit& rhs) const {return mOriginalRange==rhs.mOriginalRange;}
	bool operator==(null_t) const {return !Empty();}

private:
	R mOriginalRange;
	Utils::Optional<P1> mIsSkippedDelimiter;
	Utils::Optional<P2> mIsElementDelimiter;
	ResultOfTake<R> mFirst;
};

INTRA_WARNING_POP

template<typename R, typename P1, typename P2> forceinline Meta::EnableIf<
	IsForwardRange<R>::_,
RSplit<Meta::RemoveConstRef<R>, Meta::RemoveConstRef<P1>, Meta::RemoveConstRef<P2>>> Split(R&& range,
	P1&& isSkippedDelimiter, P2&& isElementDelimiter)
{return {Meta::Forward<R>(range), Meta::Forward<P1>(isSkippedDelimiter), Meta::Forward<P2>(isElementDelimiter)};}

template<typename T, size_t N, typename P1, typename P2> forceinline
RSplit<T, Meta::RemoveConstRef<P1>, Meta::RemoveConstRef<P2>> Split(
	T(&arr)[N], P1&& isSkippedDelimiter, P2&& isElementDelimiter)
{return Split(AsRange(arr), Meta::Forward<P1>(isSkippedDelimiter), Meta::Forward<P2>(isElementDelimiter));}

}}
