﻿#pragma once

#include "Intra/Core.h"
#include "MurmurCT.h"
#include "Murmur.h"

namespace Intra { INTRA_BEGIN
struct StringHash
{
	unsigned hash;
#ifdef _DEBUG
	union
	{
		char strEnd[12];
		const char* strLiteral;
	};
#endif

	template<int len> constexpr StringHash(const char(&str)[len]);
	constexpr StringHash(unsigned val): hash(val)
#ifdef _DEBUG
		, strLiteral(nullptr)
#endif
	{}
	constexpr StringHash(decltype(nullptr)=nullptr): hash(0)
#ifdef _DEBUG
		, strLiteral(nullptr)
#endif
	{}

	StringHash(StringView sv);

	constexpr bool operator==(const StringHash& rhs) const
	{return hash==rhs.hash;}

	bool operator!=(const StringHash& rhs) const {return !operator==(rhs);}
};

template<int len> constexpr inline StringHash::StringHash(const char(&str)[len]):
	hash(HashCT::Murmur3_32(str, 0))
#ifdef _DEBUG
	, strLiteral(str)
#endif
{}

inline StringHash::StringHash(StringView sv): hash(Hash::Murmur3_32(sv, 0))
{
#ifdef _DEBUG
	Span<char> dst = strEnd;
	WriteTo(sv.Tail(dst.Length()), dst);
	FillZeros(dst);
#endif
}
} INTRA_END