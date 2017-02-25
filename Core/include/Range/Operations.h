﻿#pragma once

#include "Meta/Type.h"
#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Concepts.h"
#include "AsRange.h"
#include "Platform/Debug.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

INTRA_DEFINE_EXPRESSION_CHECKER(HasPopFirstN, static_cast<size_t>(Meta::Val<T>().PopFirstN(size_t())));
INTRA_DEFINE_EXPRESSION_CHECKER(HasPopLastN, static_cast<size_t>(Meta::Val<T>().PopLastN(size_t())));


template<typename R> forceinline Meta::EnableIf<
	HasLength<R>::_,
size_t> Count(const R& range) {return range.Length();}

template<typename R> Meta::EnableIf<
	IsFiniteForwardRange<R>::_ && !Meta::IsConst<R>::_,
size_t> CountAdvance(R& range)
{
	size_t result=0;
	while(!range.Empty()) range.PopFirst(), result++;
	return result;
} 

template<typename R> forceinline Meta::EnableIf<
	!HasLength<R>::_ && IsAsConsumableRange<R>::_,
size_t> Count(R&& range)
{
	auto rangeCopy = Range::Forward<R>(range);
	return CountAdvance(rangeCopy);
}


template<typename R> forceinline Meta::EnableIf<
	HasLength<AsRangeResult<R>>::_,
size_t> LengthOr0(R&& range) {return Range::Forward<R>(range).Length();}


template<typename R> forceinline Meta::EnableIf<
	!HasLength<AsRangeResult<R>>::_,
size_t> LengthOr0(R&& range) {(void)range; return 0;}



template<typename R> forceinline Meta::EnableIf<
	HasSlicing<R>::_ && HasLength<R>::_ &&
	!Meta::IsConst<R>::_ && !HasPopFirstN<R>::_
> PopFirstExactly(R& range, size_t elementsToPop)
{range = range(elementsToPop, range.Length());}

template<typename R> forceinline Meta::EnableIf<
	IsInputRange<R>::_ &&
	(!HasSlicing<R>::_ || !HasLength<R>::_) && 
	!Meta::IsConst<R>::_ && !HasPopFirstN<R>::_
> PopFirstExactly(R& range, size_t elementsToPop)
{while(elementsToPop --> 0) range.PopFirst();}

template<typename R> forceinline Meta::EnableIf<
	HasPopFirstN<R>::_
> PopFirstExactly(R& range, size_t elementsToPop)
{
	size_t poppedElements = range.PopFirstN(elementsToPop);
	INTRA_ASSERT(poppedElements==elementsToPop);
	(void)poppedElements;
}


template<typename R> Meta::EnableIf<
	!Meta::IsConst<R>::_ && !HasPopFirstN<R>::_ &&
	IsFiniteInputRange<R>::_ && HasSlicing<R>::_,
size_t> PopFirstN(R& range, size_t n)
{
	const size_t l = Range::Count(range);
	const size_t elementsToPop = n<l? n: l;
	range = range(elementsToPop, l);
	return elementsToPop;
}

template<typename R> Meta::EnableIf<
	IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	!(HasSlicing<R>::_ && IsFiniteRange<R>::_) && !HasPopFirstN<R>::_,
size_t> PopFirstN(R& range, size_t n)
{
	size_t elementsToPop = 0;
	while(!range.Empty() && n!=0)
		range.PopFirst(), n--, elementsToPop++;
	return elementsToPop;
}

template<typename R> forceinline Meta::EnableIf<
	HasPopFirstN<R>::_,
size_t> PopFirstN(R& range, size_t n)
{return range.PopFirstN(n);}


template<typename R> forceinline Meta::EnableIf<
	IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	HasSlicing<R>::_ && !HasPopLastN<R>::_
> PopLastExactly(R& range, size_t elementsToPop)
{range = range(0, range.Length()-elementsToPop);}

template<typename R> forceinline Meta::EnableIf<
	IsBidirectionalRange<R>::_ && !Meta::IsConst<R>::_ &&
	!HasSlicing<R>::_ && !HasPopLastN<R>::_
> PopLastExactly(R& range, size_t elementsToPop)
{while(elementsToPop --> 0) range.PopLast();}

template<typename R> forceinline Meta::EnableIf<
	HasPopLastN<R>::_
> PopLastExactly(R& range, size_t elementsToPop)
{
	size_t poppedElements = range.PopLastN(elementsToPop);
	INTRA_ASSERT(poppedElements==elementsToPop);
	(void)poppedElements;
}


template<typename R> Meta::EnableIf<
	IsInputRange<R>::_ && !Meta::IsConst<R>::_ &&
	HasSlicing<R>::_ && !HasPopLastN<R>::_,
size_t> PopLastN(R& range, size_t n)
{
	const size_t l = Range::Count(range);
	const size_t elementsToPop = n<l? n: l;
	range = range(0, l-elementsToPop);
	return elementsToPop;
}

template<typename R> Meta::EnableIf<
	IsBidirectionalRange<R>::_ && !Meta::IsConst<R>::_ &&
	!HasSlicing<R>::_ && !HasPopLastN<R>::_,
size_t> PopLastN(R& range, size_t n)
{
	size_t elementsToPop = 0;
	while(!range.Empty() && n!=0)
		range.PopLast(), n--, elementsToPop++;
	return elementsToPop;
}

template<typename R> forceinline Meta::EnableIf<
	HasPopLastN<R>::_,
size_t> PopLastN(R& range, size_t n)
{return range.PopLastN(n);}

//! Удаляет из диапазона все элементы кроме последних n элементов.
//! \return Возвращает ссылку на себя.
template<typename R> Meta::EnableIf<
	IsForwardRange<R>::_ && !HasLength<R>::_,
R&&> TailAdvance(R&& range, size_t n)
{
	R temp = range;
	PopFirstN(temp, n);
	while(!temp.Empty())
	{
		temp.PopFirst();
		range.PopFirst();
	}
	return Meta::Forward<R>(range);
}

template<typename R> forceinline Meta::EnableIf<
	IsInputRange<R>::_ && HasLength<R>::_,
R&&> TailAdvance(R&& range, size_t n)
{
	if(range.Length()>n) PopFirstExactly(range, range.Length()-n);
	return Meta::Forward<R>(range);
}

//! Возвращает диапазон, содержащий последние n элементов данного диапазона.
template<typename R> forceinline Meta::EnableIf<
	IsAsNonInfiniteForwardRange<R>::_ && !HasSlicing<R>::_,
Meta::RemoveConstRef<AsRangeResult<R>>> Tail(R&& range, size_t n)
{
	auto rangeCopy = Range::Forward<R>(range);
	return TailAdvance(rangeCopy, n);
}

template<typename R> forceinline Meta::EnableIf<
	IsInputRange<R>::_ && HasSlicing<R>::_,
R> Tail(const R& range, size_t n)
{
	size_t len = range.Length();
	return range(len>n? len-n: 0, len);
}



template<typename R> forceinline Meta::EnableIf<
	IsAsAccessibleRange<R>::_,
AsRangeResultNoCRef<R>> Drop(R&& range)
{
	if(range.Empty()) return Range::Forward<R>(range);
	auto result = Range::Forward<R>(range);
	result.PopFirst();
	return result;
}

template<typename R> forceinline Meta::EnableIf<
	IsAsAccessibleRange<R>::_,
AsRangeResultNoCRef<R>> Drop(R&& range, size_t n)
{
	auto result = Range::Forward<R>(range);
	PopFirstN(result, n);
	return result;
}

template<typename R> forceinline Meta::EnableIf<
	IsAsAccessibleRange<R>::_,
AsRangeResultNoCRef<R>> DropExactly(R&& range)
{
	auto result = Range::Forward<R>(range);
	result.PopFirst();
	return result;
}

template<typename R> forceinline Meta::EnableIf<
	IsAsAccessibleRange<R>::_,
AsRangeResultNoCRef<R>> DropExactly(R&& range, size_t n)
{
	auto result = Range::Forward<R>(range);
	PopFirstExactly(result, n);
	return result;
}

template<typename R> forceinline Meta::EnableIf<
	IsAsInputRange<R>::_ && HasIndex<AsRangeResult<R>>::_,
ReturnValueTypeOf<R>> AtIndex(R&& range, size_t index)
{return Range::Forward<R>(range)[index];}

template<typename R> forceinline Meta::EnableIf<
	IsAsAccessibleRange<R>::_ && !HasIndex<AsRangeResult<R>>::_,
ReturnValueTypeOf<R>> AtIndex(R&& range, size_t index)
{return DropExactly(Range::Forward<R>(range), index).First();}


template<typename R> forceinline Meta::EnableIf<
	IsAsBidirectionalRange<R>::_,
Meta::RemoveConstRef<AsRangeResult<R>>> DropLast(R&& range)
{
	if(range.Empty()) return Range::Forward<R>(range);
	auto result = Range::Forward<R>(range);
	result.PopLast();
	return result;
}

template<typename R> forceinline Meta::EnableIf<
	IsAsBidirectionalRange<R>::_,
Meta::RemoveConstRef<AsRangeResult<R>>> DropLast(R&& range, size_t n)
{
	auto result = Range::Forward<R>(range);
	PopLastN(result, n);
	return result;
}

template<typename R> forceinline Meta::EnableIf<
	IsAsBidirectionalRange<R>::_,
Meta::RemoveConstRef<AsRangeResult<R>>> DropLastExactly(R&& range)
{
	auto result = Range::Forward<R>(range);
	result.PopLast();
	return result;
}

template<typename R> forceinline Meta::EnableIf<
	IsAsBidirectionalRange<R>::_,
Meta::RemoveConstRef<AsRangeResult<R>>> DropLastExactly(R&& range, size_t n)
{
	auto result = Range::Forward<R>(range);
	PopLastExactly(result, n);
	return result;
}

//! Оператор == для сравнения с null для диапазонов эквивалентен вызову Empty()
template<typename R> forceinline Meta::EnableIf<
	IsInputRange<R>::_,
bool> operator==(const R& range, null_t) {return range.Empty();}

//! Оператор == для сравнения с null для диапазонов эквивалентен вызову Empty()
template<typename R> forceinline Meta::EnableIf<
	IsInputRange<R>::_,
bool> operator==(null_t, const R& range) {return range.Empty();}

//! Оператор != для сравнения с null для диапазонов эквивалентен вызову !Empty()
template<typename R> forceinline Meta::EnableIf<
	IsInputRange<R>::_,
bool> operator!=(const R& range, null_t) {return !range.Empty();}

//! Оператор != для сравнения с null для диапазонов эквивалентен вызову !Empty()
template<typename R> forceinline Meta::EnableIf<
	IsInputRange<R>::_,
bool> operator!=(null_t, const R& range) {return !range.Empty();}

INTRA_WARNING_POP

}}
