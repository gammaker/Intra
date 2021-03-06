﻿#pragma once

#include "Range/ForwardDecls.h"
#include "Concepts/Range.h"
#include "Concepts/RangeOf.h"
#include "Range/Search/Single.h"
#include "Range/Search/Subrange.h"
#include "Take.h"

namespace Intra { namespace Range {

//TODO: Реализовать класс TakeUntilAnyResult для InputRange


//! Прочитать количество символов, предшествующих первому вхождению любого диапазона
//! из диапазона поддиапазонов subranges в этот диапазон.
//! Начало этого диапазона смещается к найденному поддиапазону или совмещается
//! с концом в случае, когда ни один поддиапазон не найден.
//! \param subranges[inout] Диапазон искомых поддиапазонов.
//! После вызова этой функции начало subranges смещается к первому совпавшему элементу.
//! Если совпадений не найдено, subranges окажется в исходном состоянии.
//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного
//! элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
//! \return Возвращает диапазон прочитанных элементов.
template<typename R, typename RWs> Meta::EnableIf<
	Concepts::IsFiniteForwardRange<RWs>::_ &&
	!Meta::IsConst<R>::_ &&
	Concepts::IsFiniteForwardRange<Concepts::ValueTypeOf<RWs>>::_ &&
	!Meta::IsConst<RWs>::_,
TakeResult<R>> TakeUntilAdvanceAnyAdvance(R&& range,
	RWs&& subranges, size_t* ioIndex, size_t* oSubrangeIndex=null)
{
	auto rangeCopy = range;
	size_t index = CountUntilAdvanceAnyAdvance(range, subranges, oSubrangeIndex);
	if(ioIndex!=null) *ioIndex += index;
	return Take(rangeCopy, index);
}

//! Последовательно удаляет элементы диапазона до тех пор, пока не:
//! 1) встретится элемент, равный одному из значений из whats
//! 2) будет достигнут конец диапазона.
//! Возвращает диапазон пройденных элементов.
template<class R, class Ws,
	typename AsWs = Concepts::RangeOfType<Ws>
> forceinline Meta::EnableIf<
	Concepts::IsNonInfiniteForwardRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Concepts::IsNonInfiniteForwardRange<AsWs>::_ &&
	Meta::IsConvertible<Concepts::ValueTypeOf<AsWs>, Concepts::ValueTypeOf<R>>::_,
TakeResult<R>> TakeUntilAdvanceAny(R& range, Ws&& whats, size_t* ioIndex=null, size_t* oWhatIndex=null)
{
	auto rangeCopy = range;
	size_t index = CountUntilAdvanceAny(range, Range::Forward<Ws>(whats), oWhatIndex);
	if(ioIndex!=null) *ioIndex += index;
	return Take(rangeCopy, index);
}

//! Последовательно просматривает элементы диапазона до тех пор, пока не:
//! 1) встретится элемент, равный одному из значений из whats
//! 2) будет достигнут конец диапазона.
//! Возвращает диапазон пройденных элементов.
template<class R, class Ws,
	typename AsR = Concepts::RangeOfType<R>,
	typename AsWs = Concepts::RangeOfType<Ws>
> forceinline Meta::EnableIf<
	Concepts::IsNonInfiniteForwardRange<AsR>::_ &&
	Concepts::IsNonInfiniteForwardRange<AsWs>::_ &&
	Meta::IsConvertible<Concepts::ValueTypeOf<AsWs>, Concepts::ValueTypeOf<AsR>>::_,
TakeResult<R>> TakeUntilAny(R&& range, Ws&& whats,
	size_t* ioIndex=null, size_t* oWhatIndex=null)
{
	size_t index = CountUntilAny(Range::Forward<R>(range), Range::Forward<Ws>(whats), oWhatIndex);
	if(ioIndex!=null) *ioIndex += index;
	return Take(range, index);
}


//! Прочитать количество символов, предшествующих первому вхождению любого диапазона из диапазона поддиапазонов subranges в этот диапазон.
//! Начало этого диапазона смещается к найденному поддиапазону или совмещается
//! с концом в случае, когда ни один поддиапазон не найден.
//! \param subranges[inout] Диапазон искомых поддиапазонов.
//! После вызова этой функции начало subranges смещается к первому совпавшему элементу.
//! Если совпадений не найдено, subranges окажется в исходном состоянии.
//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного
//! элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
//! \return Возвращает диапазон прочитанных элементов.
template<class R, class RWs,
	typename AsRWs = Concepts::RangeOfType<RWs>
> Meta::EnableIf<
	Concepts::IsNonInfiniteForwardRange<R>::_ &&
	!Meta::IsConst<R>::_ &&
	Concepts::IsNonInfiniteForwardRange<AsRWs>::_ &&
	Concepts::IsAsNonInfiniteForwardRange<Concepts::ValueTypeOf<AsRWs>>::_,
TakeResult<R>> TakeUntilAdvanceAny(R& range,
	RWs&& subranges, size_t* ioIndex=null, size_t* oSubrangeIndex=null)
{
	auto rangeCopy = range;
	size_t index = CountUntilAdvanceAny(range, Range::Forward<RWs>(subranges), oSubrangeIndex);
	if(ioIndex!=null) *ioIndex += index;
	return Take(rangeCopy, index);
}

//! Прочитать количество символов, предшествующих первому вхождению любого диапазона
//! из диапазона поддиапазонов subranges в этот диапазон.
//! \param subranges[inout] Диапазон искомых поддиапазонов.
//! После вызова этой функции начало subranges смещается к первому совпавшему элементу.
//! Если совпадений не найдено, subranges останется в исходном состоянии.
//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
//! \return Возвращает диапазон прочитанных элементов.
template<class R, class RWs,
	typename AsR = Concepts::RangeOfType<R>
> Meta::EnableIf<
	Concepts::IsNonInfiniteForwardRange<AsR>::_ &&
	Concepts::IsNonInfiniteForwardRange<RWs>::_ &&
	!Meta::IsConst<RWs>::_ &&
	Concepts::IsNonInfiniteForwardRange<Concepts::ValueTypeOf<RWs>>::_,
TakeResult<AsR>> TakeUntilAnyAdvance(R&& range,
	RWs& subranges, size_t* ioIndex=null, size_t* oSubrangeIndex=null)
{
	size_t index = CountUntilAnyAdvance(Range::RangeOf(range), subranges, oSubrangeIndex);
	if(ioIndex!=null) *ioIndex += index;
	return Take(range, index);
}

//! Прочитать количество символов, предшествующих первому вхождению любого диапазона
//! из диапазона поддиапазонов subranges в этот диапазон.
//! \param subranges[inout] Диапазон искомых поддиапазонов.
//! После вызова этой функции начало subranges смещается к первому совпавшему элементу.
//! Если совпадений не найдено, subranges останется в исходном состоянии.
//! \param oSubrangeIndex[out] Указатель на переменную, в которую будет записан индекс найденного элемента в диапазоне subranges. Если элемент не был найден, будет записано значение subranges.Count().
//! \return Возвращает диапазон прочитанных элементов.
template<class R, class RWs,
	typename AsR = Concepts::RangeOfType<R>,
	typename AsRWs = Concepts::RangeOfType<RWs>
> forceinline Meta::EnableIf<
	Concepts::IsNonInfiniteForwardRange<AsR>::_ &&
	Concepts::IsNonInfiniteForwardRange<AsRWs>::_ &&
	Concepts::IsAsNonInfiniteForwardRange<Concepts::ValueTypeOf<AsRWs>>::_,
TakeResult<R>> TakeUntilAny(R&& range, RWs&& subranges,
	size_t* ioIndex=null, size_t* oSubrangeIndex=null)
{
	const size_t index = CountUntilAny(RangeOf(range), Range::Forward<RWs>(subranges), oSubrangeIndex);
	if(ioIndex!=null) *ioIndex += index;
	return Take(range, index);
}

}}
