﻿#pragma once

#include "Cpp/Fundamental.h"
#include "Cpp/Warnings.h"

#include "Types.h"

#include "Utils/StringView.h"

namespace Intra { namespace Hash {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

uint Murmur3_32(StringView key, uint seed);
hash128 Murmur3_128_x64(StringView key, uint seed);
hash128 Murmur3_128_x32(StringView key, uint seed);
ulong64 Murmur2_64_x64(StringView key, uint seed);
ulong64 Murmur2_64_x32(StringView key, uint seed);

inline uint Murmur3_32(const char* key, uint seed)
{return Murmur3_32(StringView(key), seed);}

inline hash128 Murmur3_128_x64(const char* key, uint seed)
{return Murmur3_128_x64(StringView(key), seed);}

inline hash128 Murmur3_128_x32(const char* key, uint seed)
{return Murmur3_128_x32(StringView(key), seed);}

inline ulong64 Murmur2_64_x64(const char* key, uint seed)
{return Murmur2_64_x64(StringView(key), seed);}

inline ulong64 Murmur2_64_x32(const char* key, uint seed)
{return Murmur2_64_x32(StringView(key), seed);}

INTRA_WARNING_POP

}}
