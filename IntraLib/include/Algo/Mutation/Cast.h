#pragma once

#include "Range/Concepts.h"
#include "Range/ArrayRange.h"
#include "Copy.h"

namespace Intra { namespace Algo {

template<typename R, typename OR> Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsOutputRange<OR>::_ &&
	!Range::ValueTypeEquals<R, OR>::_
> CastAdvanceToAdvance(R&& src, OR&& dst)
{
	while(!src.Empty())
	{
		dst.Put(Range::ValueTypeOf<OR>(src.First()));
		src.PopFirst();
	}
}

//���������������� ����������
void CastAdvanceToAdvance(ArrayRange<const float>& src, ArrayRange<short>& dst);
forceinline void CastAdvanceToAdvance(ArrayRange<const float>&& src, ArrayRange<short>& dst)
{CastAdvanceToAdvance(src, dst);}
forceinline void CastAdvanceToAdvance(ArrayRange<const float>& src, ArrayRange<short>&& dst)
{CastAdvanceToAdvance(src, dst);}
forceinline void CastAdvanceToAdvance(ArrayRange<const float>&& src, ArrayRange<short>&& dst)
{CastAdvanceToAdvance(src, dst);}

template<typename R, typename OR> Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsOutputRange<OR>::_ &&
	Range::ValueTypeEquals<R, OR>::_
> CastAdvanceToAdvance(R&& src, OR&& dst)
{CopyAdvanceToAdvance(src, dst);}

template<typename R, typename OR> Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	Range::IsOutputRange<OR>::_
> CastAdvanceTo(R&& src, const OR& dst)
{CastAdvanceToAdvance(src, OR(dst));}

template<typename R, typename OR> Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ &&
	Range::IsOutputRange<OR>::_
> CastToAdvance(const R& src, OR&& dst)
{CastAdvanceToAdvance(R(src), dst);}

template<typename R, typename OR> Meta::EnableIf<
	Range::IsFiniteInputRange<R>::_ &&
	Range::IsOutputRange<OR>::_
> CastTo(const R& src, const OR& dst)
{CastAdvanceToAdvance(R(src), OR(dst));}



//! ����������� ������ �������� ������� �������� �� NumericLimits<From>::Max
template<typename To, typename From> Meta::EnableIf<
	Meta::IsFloatType<To>::_ && Meta::IsIntegralType<From>::_
> CastToNormalized(ArrayRange<To> dst, ArrayRange<const From> src)
{
	INTRA_ASSERT(dst.Length()==src.Length());
	while(!dst.Empty())
	{
		dst.First() = To(src.First()) / To(Meta::NumericLimits<From>::Max());
		dst.PopFirst();
		src.PopFirst();
	}
}

//! ��������� ������ dst ���������� �� src, ����������� �� NumericLimits<To>::Max.
//! ��������������, ��� -1.0 <= src[i] <= 1.0. ����� ��������� ������������.
template<typename To, typename From> Meta::EnableIf<
	Meta::IsFloatType<From>::_ && Meta::IsIntegralType<To>::_
> CastFromNormalized(ArrayRange<To> dst, ArrayRange<const From> src)
{
	INTRA_ASSERT(dst.Length()==src.Length());
	while(!dst.Empty())
	{
		dst.First() = To(src.First() * Meta::NumericLimits<From>::Max());
		dst.PopFirst();
		src.PopFirst();
	}
}

}}
