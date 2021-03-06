#pragma once

#include "Cpp/Features.h"
#include "Concepts/Range.h"

namespace Intra { namespace Range {

template<typename R> struct RangeForwardIterator
{
	typedef Concepts::ValueTypeOf<R> value_type;
	typedef Concepts::ReturnValueTypeOf<R> return_value_type;
	typedef return_value_type& reference;
	typedef value_type* pointer;
	typedef intptr difference_type;

	forceinline RangeForwardIterator(null_t=null) {}
	forceinline RangeForwardIterator(const R& range): Range(range) {}
	forceinline RangeForwardIterator& operator++() { Range.PopFirst(); return *this;}
	forceinline RangeForwardIterator operator++(int) {auto copy = Range; Range.PopFirst(); return copy;}
	forceinline return_value_type operator*() const {return Range.First();}
	forceinline Meta::RemoveReference<return_value_type>* operator->() const {return &Range.First();}

	forceinline bool operator==(const RangeForwardIterator& rhs) const {return Range==rhs.Range;}
	forceinline bool operator!=(const RangeForwardIterator& rhs) const {return !operator==(rhs);}
	forceinline bool operator==(null_t) const {return Range.Empty();}

	R Range;
};

}
using Range::RangeForwardIterator;

}
