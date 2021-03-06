﻿#pragma once

#include "Cpp/Warnings.h"
#include "Meta/Type.h"
#include "Concepts/Range.h"
#include "Range/Comparison/StartsWith.h"
#include "Range/Search/Subrange.h"
#include "Range/Decorators/Take.h"
#include "Range/Generators/Null.h"
#include "Range/Operations.h"
#include "Meta/GetField.h"

namespace Intra { namespace Range {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

template<typename R, typename OB, typename CB, typename ST, typename CBP, typename RCBP> Meta::EnableIf<
	Concepts::IsNonInfiniteForwardRange<R>::_ &&
	Concepts::IsAsNonInfiniteForwardRange<OB>::_ &&
	Concepts::IsAsNonInfiniteForwardRange<CB>::_ &&
	Concepts::IsAsNonInfiniteForwardRange<ST>::_ &&
	Concepts::IsAsFiniteForwardRange<CBP>::_ &&
	Concepts::IsAsFiniteForwardRange<RCBP>::_,
TakeResult<R>> TakeRecursiveBlockAdvance(R& range, int& counter, size_t* ioIndex,
	const OB& openingBracket, const CB& closingBracket, const ST& stopToken,
	const CBP& commentBlocks, const RCBP& recursiveCommentBlocks)
{
	using namespace Range;
	using Range::Count;
	typedef Concepts::ValueTypeOf<R> T;

	auto start = range;
	size_t index = 0;
	const size_t openingBracketLen = Count(openingBracket);
	const size_t closingBracketLen = Count(closingBracket);
	const size_t stopTokenLen = Count(stopToken);
	while(!range.Empty() && counter!=0)
	{
		if(openingBracketLen!=0 && StartsWith(range, openingBracket))
		{
			counter++;
			PopFirstExactly(range, openingBracketLen);
			index += openingBracketLen;
			continue;
		}

		if(closingBracketLen!=0 && StartsWith(range, closingBracket))
		{
			counter--;
			PopFirstExactly(range, closingBracketLen);
			index += closingBracketLen;
			continue;
		}

		if(stopTokenLen!=0 && StartsWith(range, stopToken))
		{
			PopFirstExactly(range, stopTokenLen);
			index += stopTokenLen;
			break;
		}

		bool commentFound = false;
		for(auto cblocks = RangeOf(commentBlocks); !cblocks.Empty(); cblocks.PopFirst())
		{
			auto commentBlockBegin = Meta::Get<0>(cblocks.First());
			auto commentBlockEnd = Meta::Get<1>(cblocks.First());
			commentFound = StartsWith(range, commentBlockBegin);
			if(!commentFound) continue;

			const size_t commentBlockOpeningLen = Count(commentBlockBegin);
			const size_t commentBlockClosingLen = Count(commentBlockEnd);
			PopFirstN(range, commentBlockOpeningLen);
			index += commentBlockOpeningLen;
			FindAdvance(range, commentBlockEnd, &index);
			PopFirstN(range, commentBlockClosingLen);
			index += commentBlockClosingLen;
			break;
		}
		if(commentFound) continue;

		for(auto reccblocks = RangeOf(recursiveCommentBlocks); !reccblocks.Empty(); reccblocks.PopFirst())
		{
			auto commentBlockBegin = Meta::Get<0>(reccblocks.First());
			auto commentBlockEnd = Meta::Get<1>(reccblocks.First());
			commentFound = StartsWith(range, commentBlockBegin);
			if(!commentFound) continue;

			int commentCounter = 1;
			TakeRecursiveBlockAdvance(range, commentCounter, &index,
				commentBlockBegin, commentBlockEnd, NullRange<T>(),
				NullRange<Meta::Pair<NullRange<T>, NullRange<T>>>(),
				NullRange<Meta::Pair<NullRange<T>, NullRange<T>>>());
			break;
		}
		if(commentFound) continue;

		range.PopFirst();
	}
	if(ioIndex!=null) *ioIndex += index;
	return Take(start, index);
}

template<typename R, typename OB, typename CB, typename ST> Meta::EnableIf<
	Concepts::IsNonInfiniteForwardRange<R>::_ &&
	Concepts::IsAsNonInfiniteForwardRange<OB>::_ &&
	Concepts::IsAsNonInfiniteForwardRange<CB>::_ &&
	Concepts::IsAsNonInfiniteForwardRange<ST>::_,
TakeResult<R>> TakeRecursiveBlockAdvance(R& range, int& counter, size_t* ioIndex,
	const OB& openingBracket, const CB& closingBracket, const ST& stopToken)
{
	using namespace Range;
	typedef Concepts::ValueTypeOf<R> T;

	return TakeRecursiveBlockAdvance(range, counter, ioIndex,
		openingBracket, closingBracket, stopToken,
		NullRange<Meta::Pair<NullRange<T>, NullRange<T>>>(),
		NullRange<Meta::Pair<NullRange<T>, NullRange<T>>>());
}

INTRA_WARNING_POP

}}
