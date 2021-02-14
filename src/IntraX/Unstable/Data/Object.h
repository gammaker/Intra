﻿#pragma once

#include "Intra/Range/StringView.h"
#include "IntraX/Container/Sequential/Array.h"
#include "IntraX/Container/Sequential/String.h"
#include "IntraX/Container/Associative/LinearMap.h"
#include "Intra/Range/Polymorphic/FiniteRandomAccessRange.h"
#include "IntraX/Unstable/Data/Serialization/TextSerializer.h"

namespace Intra { INTRA_BEGIN
struct Object
{
	template<typename T> using StringMap = LinearMap<String, T>;

	StringMap<double> Numbers;
	StringMap<String> Strings;
	StringMap<Object> Objects;
	StringMap<Array<double>> NumberArrays;
	StringMap<Array<String>> StringArrays;
	StringMap<Array<Object>> ObjectArrays;

	Object(const Object&);
	Object(Object&&);
	Object& operator=(const Object&);
	Object& operator=(Object&&);

	Object(decltype(nullptr)=nullptr) {}
	Object& operator=(decltype(nullptr))
	{
		Numbers = nullptr;
		Strings = nullptr;
		Objects = nullptr;
		NumberArrays = nullptr;
		StringArrays = nullptr;
		ObjectArrays = nullptr;
		return *this;
	}

	bool operator==(decltype(nullptr)) const
	{
		return Numbers.Empty() && Strings.Empty() && Objects.Empty() &&
			NumberArrays.Empty() && StringArrays.Empty() && ObjectArrays.Empty();
	}

	INTRA_FORCEINLINE bool operator!=(decltype(nullptr)) const {return !operator==(nullptr);}

	template<typename T> INTRA_FORCEINLINE Requires<
		CBasicFloatingPoint<T> || CBasicIntegral<T>,
	T> Get(StringView key, T defaultValue=T()) const
	{return T(Numbers.Get(key, double(defaultValue)));}

	/// Возвращает строку по ключу key. Если строка не найдена, то ищет число и переводит в строку.
	/// Если ни строка, ни число не найдены, возвращает defaultValue.
	String GetString(StringView key, StringView defaultValue=nullptr) const
	{
		const size_t stringIndex = Strings.FindIndex(key);
		if(stringIndex != Strings.Count()) return Strings.Value(stringIndex);
		const size_t numIndex = Numbers.FindIndex(key);
		if(numIndex != Numbers.Count()) return StringOf(Numbers.Value(numIndex));
		return defaultValue;
	}
};

inline Object::Object(const Object&) = default;
inline Object::Object(Object&&) = default;
inline Object& Object::operator=(const Object&) = default;
inline Object& Object::operator=(Object&&) = default;

/*template<typename O>
GenericTextSerializer<O>& operator<<(GenericTextSerializer<O>& serializer, const IConstObject& src)
{
	//serializer.
	return serializer;
}*/

} INTRA_END