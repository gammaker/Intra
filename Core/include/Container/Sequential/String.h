﻿#pragma once

#include "Platform/CppFeatures.h"
#include "Platform/CppWarnings.h"
#include "Platform/Debug.h"
#include "Meta/Mixins.h"
#include "Container/ForwardDecls.h"
#include "Container/Operations.hh"
#include "Container/Concepts.h"
#include "Range/Generators/ArrayRange.h"
#include "Range/Generators/StringView.h"
#include "Algo/String/CStr.h"
#include "Memory/Memory.h"
#include "Memory/Allocator.hh"
#include "Range/Special/Unicode.h"
#include "Data/Reflection.h"
#include "Algo/Mutation/Fill.h"
#include "Algo/Mutation/Copy.h"
#include "Algo/Mutation/ReplaceSubrange.h"
#include "Range/Decorators/TakeUntil.h"
#include "Range/Compositors/Zip.h"
#include "StringFormatter.h"
#include "Range/Output/Inserter.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
INTRA_WARNING_DISABLE_SIGN_CONVERSION


namespace Intra { namespace Container {

template<typename Char> class GenericString
{
public:
	GenericString(const Char* str):
		GenericString(str, (str==null)? 0: Algo::CStringLength(str)) {}

	GenericString(null_t=null) {resetToEmptySsoWithoutFreeing();}

	explicit GenericString(const Char* str, size_t strLength): GenericString(null)
	{
		SetLengthUninitialized(strLength);
		C::memcpy(Data(), str, strLength*sizeof(Char));
	}

	explicit forceinline GenericString(const Char* startPtr, const Char* endPtr):
		GenericString(startPtr, size_t(endPtr-startPtr)) {}
	
	explicit forceinline GenericString(size_t initialLength, Char filler): GenericString(null)
	{SetLength(initialLength, filler);}

	template<typename R=StringView, typename AsR=Range::AsRangeResult<R>, typename = Meta::EnableIf<
		Range::IsConsumableRangeOf<AsR, Char>::_ &&
		Meta::IsCharType<Range::ValueTypeOf<AsR>>::_
	>> forceinline GenericString(R&& rhs): GenericString(null)
	{
		SetLengthUninitialized(Range::Count(rhs));
		Algo::CopyTo(Range::Forward<R>(rhs), AsRange());
	}

	forceinline GenericString(const GenericString& rhs):
		GenericString(rhs.AsConstRange()) {}
	
	forceinline GenericString(GenericString&& rhs):
		m(rhs.m) {rhs.m.Capacity = SSO_CAPACITY_FIELD_FOR_EMPTY;}
	
	~GenericString()
	{
		if(IsHeapAllocated()) freeLongData();
	}

	//! Получить диапазон из UTF-32 кодов символов
	//UTF8 ByChar() const {return UTF8(Data(), Length());}

	//! Получить подстроку, начиная с кодовой единицы с индексом start и заканчивая end.
	forceinline GenericStringView<const Char> operator()(
		size_t startIndex, size_t endIndex) const
	{return View(startIndex, endIndex);}

	//! Получить подстроку, начиная с кодовой единицы с индексом start и заканчивая end.
	forceinline GenericStringView<const Char> operator()(
		Range::RelativeIndex startIndex, Range::RelativeIndex endIndex) const
	{return View(startIndex, endIndex);}

	//! Получить подстроку, начиная с кодовой единицы с индексом start и до конца.
	forceinline GenericStringView<const Char> operator()(
		Range::RelativeIndex startIndex, Range::RelativeIndexEnd) const
	{return View(startIndex, $);}

	//! Получить подстроку, начиная с кодовой единицы с индексом start и до конца.
	forceinline GenericStringView<const Char> operator()(
		size_t startIndex, Range::RelativeIndexEnd) const
	{return View(startIndex, $);}


	//! Получить указатель на C-строку для передачи в различные C API.
	/**
	Полученный указатель временный и может стать некорректным при любой модификации или удалении этого экземпляра класса.
	Эту функцию следует использовать осторожно с временными объектами:
	const char* cstr = String("a").CStr(); //Ошибка: cstr - висячий указатель.
	strcpy(dst, String("a").CStr()); //Ок: указатель до возврата strcpy корректен
	**/
	const Char* CStr()
	{
		size_t len = Length();
		if(!IsHeapAllocated())
		{
			mBuffer[len] = '\0';
			return mBuffer;
		}
		RequireSpace(1);
		auto data = Data();
		data[len] = '\0';
		return data;
	}

	//! Возвращает диапазон символов строки.
	//!@{
	/**
	Возвращаемый диапазон корректен только до тех пор, пока длина строки не изменится или экземпляр класса не будет удалён.
	Эту функцию следует использовать осторожно с временными объектами:
	auto range = String("a").AsRange(); //Ошибка: range содержит висячие указатели.
	foo(String("a").AsRange()); //Ок: диапазон до возврата из foo корректен
	**/
	forceinline GenericStringView<Char> AsRange() {return {Data(), Length()};}
	forceinline GenericStringView<const Char> AsRange() const {return {Data(), Length()};}
	forceinline GenericStringView<const Char> AsConstRange() const {return {Data(), Length()};}
	//!@}

	//! Присваивание строк
	//!@{
	GenericString& operator=(GenericString&& rhs)
	{
		if(IsHeapAllocated()) freeLongData();
		m = rhs.m;
		rhs.m.Capacity = SSO_CAPACITY_FIELD_FOR_EMPTY;
		return *this;
	}

	GenericString& operator=(GenericStringView<const Char> rhs)
	{
		SetLengthUninitialized(0);
		SetLengthUninitialized(rhs.Length());
		C::memcpy(Data(), rhs.Data(), rhs.Length()*sizeof(Char));
		return *this;
	}

	forceinline GenericString& operator=(const GenericString& rhs)
	{return operator=(rhs.AsConstRange());}

	forceinline GenericString& operator=(const Char* rhs)
	{return operator=(GenericStringView<const Char>(rhs));}
	
	template<typename R> forceinline Meta::EnableIf<
		Range::IsAsArrayRange<R>::_,
	GenericString&> operator=(R&& rhs)
	{return operator=(GenericStringView<const Char>(rhs));}

	template<typename R> forceinline Meta::EnableIf<
		!Range::IsAsArrayRange<R>::_ && Range::IsAsConsumableRangeOf<R, Char>::_,
	GenericString&> operator=(R&& rhs)
	{
		SetLengthUninitialized(0);
		SetLengthUninitialized(Range::Count(rhs));
		Algo::CopyTo(Range::Forward<R>(rhs), AsRange());
		return *this;
	}

	GenericString& operator=(null_t)
	{
		if(IsHeapAllocated())
		{
			freeLongData();
			resetToEmptySsoWithoutFreeing();
		}
		return *this;
	}
	//!@}

	forceinline GenericString& operator+=(StringFormatter<GenericString>& rhs) {return operator+=(*rhs);}
	forceinline GenericString& operator+=(Char rhs)
	{
		const size_t oldLen = Length();
		const bool wasAllocated = IsHeapAllocated();
		if(!wasAllocated && oldLen<SSO_BUFFER_CAPACITY_CHARS)
		{
			shortLenIncrement();
			mBuffer[oldLen]=rhs;
		}
		else
		{
			if(!wasAllocated || getLongCapacity()==oldLen)
				Reallocate(oldLen+1+oldLen/2);
			m.Data[m.Len++] = rhs;
		}
		return *this;
	}

	forceinline bool operator==(null_t) const {return Empty();}

	forceinline bool operator>(const GenericStringView<const Char>& rhs) const {return View()>rhs;}
	forceinline bool operator<(const Char* rhs) const {return View()<rhs;}
	forceinline bool operator<(const GenericStringView<const Char>& rhs) const {return View()<rhs;}


	//! Убедиться, что буфер строки имеет достаточно свободного места для хранения minCapacity символов.
	forceinline void Reserve(size_t minCapacity)
	{
		if(Capacity()<minCapacity)
			Reallocate(minCapacity+Length()/2);
	}


	//! Изменить ёмкость строки, чтобы вместить newCapacity символов.
	void Reallocate(size_t newCapacity)
	{
		const bool wasAllocated = IsHeapAllocated();
		if(!wasAllocated && newCapacity<=SSO_BUFFER_CAPACITY_CHARS) return;
		const size_t len = Length();
		if(newCapacity<len) newCapacity=len;
		Char* newData;
		if(newCapacity>SSO_BUFFER_CAPACITY_CHARS)
		{
			size_t newCapacityInBytes = newCapacity*sizeof(Char);
			newData = Memory::GlobalHeap.Allocate(newCapacityInBytes, INTRA_SOURCE_INFO);
			newCapacity = newCapacityInBytes/sizeof(Char);
		}
		else
		{
			newData = mBuffer;
			setShortLength(len);
		}
		Char* const oldData = m.Data;
		const size_t oldLongCapacity = getLongCapacity();
		C::memcpy(newData, Data(), len*sizeof(Char));
		if(wasAllocated) Memory::GlobalHeap.Free(oldData, oldLongCapacity*sizeof(Char));
		if(newData!=mBuffer)
		{
			m.Data = newData;
			setLongCapacity(newCapacity);
			m.Len = len;
		}
	}

	//! Если ёмкость буфера вмещает больше, чем длина строки более, чем на 20%, она уменьшается, чтобы совпадать с длиной.
	//! Реальный размер буфера может получиться больше, чем длина строки. Это зависит от используемого аллокатора.
	forceinline void TrimExcessCapacity()
	{if(Capacity()>Length()*5/4) Reallocate(Length());}

	//! Убедиться, что буфер строки имеет достаточно свободного места для добавления minSpace символов в конец.
	forceinline void RequireSpace(size_t minSpace) {Reserve(Length()+minSpace);}

	//! Установить длину строки в newLen.
	//! Если newLen<Length(), то лишние символы отбрасываются.
	//! Если newLen>Length(), то новые символы заполняются с помощью filler.
	void SetLength(size_t newLen, Char filler='\0')
	{
		const size_t oldLength = Length();
		SetLengthUninitialized(newLen);
		if(newLen>oldLength) Algo::Fill(View(oldLength, $), filler);
	}

	forceinline void SetCount(size_t newLen, Char filler='\0') {SetLength(newLen, filler);}

	//! Установить длину строки в newLen.
	//! Если newLen<Length(), то лишние символы отбрасываются.
	//! Если newLen>Length(), то новые символы остаются неинициализированными.
	forceinline void SetLengthUninitialized(size_t newLen)
	{
		Reserve(newLen);
		if(IsHeapAllocated()) m.Len = newLen;
		else setShortLength(newLen);
	}

	//! Установить длину строки в newLen.
	//! Если newLen<Length(), то лишние символы отбрасываются.
	//! Если newLen>Length(), то новые символы остаются неинициализированными.
	forceinline void SetCountUninitialized(size_t newLen)
	{SetLengthUninitialized(newLen);}

	//! Количество символов, которое может уместить текущий буфер строки без перераспределения памяти.
	forceinline size_t Capacity() const
	{return IsHeapAllocated()? getLongCapacity(): SSO_BUFFER_CAPACITY_CHARS;}

	forceinline Char* Data() {return IsHeapAllocated()? m.Data: mBuffer;}
	forceinline const Char* Data() const {return IsHeapAllocated()? m.Data: mBuffer;}
	forceinline Char* End() {return Data()+Length();}
	forceinline const Char* End() const {return Data()+Length();}

	forceinline Char& operator[](size_t index)
	{
		INTRA_ASSERT(index<Length());
		return Data()[index];
	}

	forceinline const Char& operator[](size_t index) const
	{
		INTRA_ASSERT(index<Length());
		return Data()[index];
	}

	forceinline bool Empty() const
	{return IsHeapAllocated()? m.Len==0: emptyShort();}

	forceinline Char& First() {INTRA_ASSERT(!Empty()); return *Data();}
	forceinline const Char& First() const {INTRA_ASSERT(!Empty()); return *Data();}
	forceinline Char& Last() {INTRA_ASSERT(!Empty()); return Data()[Length()-1];}
	forceinline const Char& Last() const {INTRA_ASSERT(!Empty()); return Data()[Length()-1];}

	forceinline void PopLast()
	{
		INTRA_ASSERT(!Empty());
		if(IsHeapAllocated()) m.Len--;
		else shortLenDecrement();
	}

	forceinline size_t Length() const
	{return IsHeapAllocated()? m.Len: getShortLength();}

	//! Заменить все вхождения subStr на newSubStr
	static GenericString ReplaceAll(GenericStringView<const Char> str,
		GenericStringView<const Char> subStr, GenericStringView<const Char> newSubStr)
	{
		GenericString result;
		Range::CountRange<Char> counter;
		Algo::MultiReplaceToAdvance(str, counter, Range::AsRange({Meta::TupleL(subStr, newSubStr)}));
		result.SetLengthUninitialized(counter.Counter);
		Algo::MultiReplaceToAdvance(str, result.AsRange(), Range::AsRange({Meta::TupleL(subStr, newSubStr)}));
		return result;
	}

	static GenericString ReplaceAll(GenericStringView<const Char> str, Char c, Char newc)
	{
		if(c==newc) return str;
		GenericString result;
		result.SetLengthUninitialized(str.Length());
		for(size_t i = 0; i<str.Length(); i++)
			result[i] = str[i]==c? newc: str[i];
		return result;
	}

	static GenericString MultiReplace(GenericStringView<const Char> str,
		ArrayRange<const GenericStringView<const Char>> subStrs,
		ArrayRange<const GenericStringView<const Char>> newSubStrs)
	{
		GenericString result;
		Range::CountRange<Char> counter;
		Algo::MultiReplaceToAdvance(str, counter, Range::Zip(subStrs, newSubStrs));
		result.SetLengthUninitialized(counter.Counter);
		Algo::MultiReplaceTo(str, result, Range::Zip(subStrs, newSubStrs));
		return result;
	}

	//! Повторить строку n раз
	static GenericString Repeat(GenericStringView<const Char> str, size_t n)
	{
		if(str.Empty()) return null;
		GenericString result;
		result.SetLengthUninitialized(str.Length()*n);
		Algo::FillPattern(result, str);
		return result;
	}

	forceinline void AddLast(Char c) {operator+=(c);}

	//! @defgroup BList_STL_Interface STL-подобный интерфейс для BList
	//! Этот интерфейс предназначен для совместимости с обобщённым контейнеро-независимым кодом.
	//! Использовать напрямую этот интерфейс не рекомендуется.
	//!@{
	typedef char* iterator;
	typedef const char* const_iterator;
	typedef Char value_type;

	forceinline Char* begin() {return Data();}
	forceinline Char* end() {return Data()+Length();}
	forceinline const Char* begin() const {return Data();}
	forceinline const Char* end() const {return Data()+Length();}

	forceinline void push_back(Char c) {operator+=(c);}
	forceinline void reserve(size_t capacity) {Reserve(capacity);}
	forceinline void resize(size_t newLength, Char filler='\0') {SetLength(newLength, filler);}
	forceinline size_t size() const {return Length();}
	forceinline size_t length() const {return Length();}
	forceinline const Char* c_str() {return CStr();} //Не const, в отличие от STL
	forceinline GenericString substr(size_t start, size_t count) const {return operator()(start, start+count);}
	forceinline bool empty() const {return Empty();}
	forceinline Char* data() {return Data();}
	forceinline const Char* data() const {return Data();}

	forceinline GenericString& append(const GenericString& str)
	{*this += str.AsRange();}

	forceinline GenericString& append(const GenericString& str, size_t subpos, size_t sublen)
	{*this += str(subpos, subpos+sublen);}

	forceinline GenericString& append(const Char* s)
	{*this += GenericStringView<const Char>(s);}

	forceinline GenericString& append(const Char* s, size_t n)
	{*this += GenericStringView<const Char>(s, n);}

	GenericString& append(size_t n, Char c)
	{
		size_t oldLen = Length();
		SetLengthUninitialized(oldLen+n);
		Algo::Fill(View(oldLen, $), c);
	}

	template<class InputIt> forceinline GenericString& append(InputIt firstIt, InputIt endIt)
	{while(firstIt!=endIt) *this += *endIt++;}
	//!@}


	//! Форматирование строки
	//! \param format Строка, содержащая метки <^>, в которые будут подставляться аргументы.
	//! \param () Используйте скобки для передачи параметров и указания их форматирования
	//! \returns Прокси-объект для форматирования, неявно преобразующийся к String.
	static forceinline StringFormatter<GenericString> Format(GenericStringView<const Char> format=null)
	{return StringFormatter<GenericString>(format);}

	forceinline GenericStringView<Char> View()
	{return {Data(), Length()};}
	
	forceinline GenericStringView<Char> View(size_t index, Range::RelativeIndexEnd)
	{
		INTRA_ASSERT(index <= Length());
		return {Data()+index, Data()+Length()};
	}
	
	forceinline GenericStringView<Char> View(size_t startIndex, size_t endIndex)
	{
		INTRA_ASSERT(startIndex <= endIndex);
		INTRA_ASSERT(endIndex <= Length());
		return {Data()+startIndex, Data()+endIndex};
	}

	GenericStringView<Char> View(Range::RelativeIndex startIndex, Range::RelativeIndex endIndex)
	{
		const size_t len = Length();
		return View(startIndex.GetRealIndex(len), endIndex.GetRealIndex(len));
	}

	forceinline GenericStringView<const Char> View() const
	{return {Data(), Length()};}
	
	forceinline GenericStringView<const Char> View(size_t index, Range::RelativeIndexEnd) const
	{
		INTRA_ASSERT(index <= Length());
		return {Data()+index, Data()+Length()};
	}
	
	forceinline GenericStringView<const Char> View(size_t startIndex, size_t endIndex) const
	{
		INTRA_ASSERT(startIndex <= endIndex);
		INTRA_ASSERT(endIndex <= Length());
		return {Data()+startIndex, Data()+endIndex};
	}

	GenericStringView<const Char> View(Range::RelativeIndex startIndex, Range::RelativeIndex endIndex) const
	{
		const size_t len = Length();
		return View(startIndex.GetRealIndex(len), endIndex.GetRealIndex(len));
	}

	forceinline bool IsHeapAllocated() const {return (m.Capacity & SSO_LONG_BIT_MASK)!=0;}

private:
	struct M
	{
		mutable Char* Data;
		size_t Len;
		size_t Capacity;
	};

	union
	{
		M m;
		Char mBuffer[sizeof(M)/sizeof(Char)];
	};

	enum: size_t
	{
		SSO_LE = INTRA_PLATFORM_ENDIANESS==INTRA_PLATFORM_ENDIANESS_LittleEndian,
		SSO_LONG_BIT_MASK = SSO_LE? //Выбираем бит, который будет означать, выделена ли память в куче. Он должен находиться в последнем элементе mBuffer.
			(size_t(1) << (sizeof(size_t)*8-1)): //В little-endian лучше всего подходит старший бит m.Capacity.
			1, //В big-endian лучше всего подходит младший бит m.Capacity.
		SSO_CAPACITY_MASK = ~SSO_LONG_BIT_MASK,
		SSO_CAPACITY_RIGHT_SHIFT = 1-SSO_LE, //В little-endian сдвига нет, а в big-endian нужно убрать младший бит, не относящийся к ёмкости выделенного буфера
		SSO_SHORT_SIZE_SHIFT = 1-SSO_LE, //Когда строка размещается в SSO, в последнем элементе mBuffer содержится обратная длина строки. Но в big-endian младший бит занят, поэтому нужен сдвиг
		SSO_BUFFER_CAPACITY_CHARS = sizeof(M)/sizeof(Char)-1, //В режиме SSO используется все байты объекта кроме последнего, который может быть только терминирующим нулём
		SSO_CAPACITY_FIELD_FOR_EMPTY = SSO_LE? //Значение m.Capacity, соответствующее пустой строке. Нужно для конструктора по уиолчанию
			SSO_BUFFER_CAPACITY_CHARS << (sizeof(size_t)-sizeof(Char))*8:
			SSO_BUFFER_CAPACITY_CHARS << 1
	};

	forceinline void setShortLength(size_t len)
	{mBuffer[SSO_BUFFER_CAPACITY_CHARS] = Char((SSO_BUFFER_CAPACITY_CHARS-len) << SSO_SHORT_SIZE_SHIFT);}

	forceinline size_t getShortLength() const
	{return SSO_BUFFER_CAPACITY_CHARS - (mBuffer[SSO_BUFFER_CAPACITY_CHARS] >> SSO_SHORT_SIZE_SHIFT);}

	forceinline void setLongCapacity(size_t newCapacity)
	{m.Capacity = (newCapacity << SSO_CAPACITY_RIGHT_SHIFT)|SSO_LONG_BIT_MASK;}

	forceinline size_t getLongCapacity() const
	{return (m.Capacity & SSO_CAPACITY_MASK) >> SSO_CAPACITY_RIGHT_SHIFT;}

	forceinline void shortLenDecrement()
	{mBuffer[SSO_BUFFER_CAPACITY_CHARS] = Char(mBuffer[SSO_BUFFER_CAPACITY_CHARS] + (1 << SSO_SHORT_SIZE_SHIFT));}

	forceinline void shortLenIncrement()
	{mBuffer[SSO_BUFFER_CAPACITY_CHARS] = Char(mBuffer[SSO_BUFFER_CAPACITY_CHARS] - (1 << SSO_SHORT_SIZE_SHIFT));}

	forceinline bool emptyShort() const 
	{return mBuffer[SSO_BUFFER_CAPACITY_CHARS]==Char(SSO_BUFFER_CAPACITY_CHARS << SSO_SHORT_SIZE_SHIFT);}

	forceinline void resetToEmptySsoWithoutFreeing()
	{m.Capacity = SSO_CAPACITY_FIELD_FOR_EMPTY;}

	forceinline void freeLongData()
	{Memory::GlobalHeap.Free(m.Data, getLongCapacity()*sizeof(Char));}
};

template<typename Char> forceinline
GenericString<Char> operator+(GenericString<Char>&& lhs, GenericStringView<const Char> rhs)
{return Meta::Move(lhs+=rhs);}

template<typename R, typename Char=Range::ValueTypeOfAs<R>> forceinline Meta::EnableIf<
	Range::IsAsArrayRange<R>::_ && !Meta::TypeEquals<R, GenericStringView<const Char>>::_,
GenericString<Char>> operator+(GenericString<Char>&& lhs, R&& rhs)
{return Meta::Move(lhs+=GenericStringView<const Char>(rhs));}


template<typename Char> forceinline
GenericString<Char> operator+(GenericString<Char>&& lhs, Char rhs)
{return Meta::Move(lhs+=rhs);}

#ifdef INTRA_USER_DEFINED_LITERALS_SUPPORT
forceinline String operator ""_s(const char* str, size_t len)
{return String(str, len);}

forceinline WString operator ""_w(const wchar* str, size_t len)
{return WString(str, len);}

forceinline DString operator ""_d(const dchar* str, size_t len)
{return DString(str, len);}
#endif


template<typename T, typename... Args> forceinline String ToString(const T& value, Args&&... args)
{return String::Format()(value, Meta::Forward<Args>(args)...);}

template<typename T, size_t N, typename... Args> forceinline String ToString(T(&value)[N], Args&&... args)
{return String::Format()(value, Meta::Forward<Args>(args)...);}

forceinline const String& ToString(const String& value) {return value;}
forceinline StringView ToString(const StringView& value) {return value;}
forceinline StringView ToString(const char* value) {return StringView(value);}
template<size_t N> forceinline StringView ToString(const char(&value)[N]) {return StringView(value);}


template<typename T, typename... Args> forceinline WString ToWString(const T& value, Args&&... args)
{return WString::Format()(value, Meta::Forward<Args>(args)...);}

forceinline const WString& ToWString(const WString& value) {return value;}
forceinline WStringView ToWString(const WStringView& value) {return value;}
forceinline WStringView ToWString(const wchar* value) {return WStringView(value);}
template<size_t N> forceinline WStringView ToWString(const wchar(&value)[N]) {return WStringView(value);}


template<typename T, typename... Args> forceinline DString ToDString(const T& value, Args&&... args)
{return DString::Format()(value, Meta::Forward<Args>(args)...);}

forceinline const DString& ToDString(const DString& value) {return value;}
forceinline DStringView ToDString(const DStringView& value) {return value;}
forceinline DStringView ToDString(const dchar* value) {return DStringView(value);}
template<size_t N> forceinline DStringView ToDString(const dchar(&value)[N]) {return DStringView(value);}

}
using Container::ToString;
using Container::ToWString;
using Container::ToDString;

namespace Meta {

template<typename Char>
struct IsTriviallyRelocatable<GenericString<Char>>: TrueType {};


namespace D {
template<typename Char> struct CommonTypeRef<GenericStringView<Char>, GenericString<Char>> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericStringView<Char>&, GenericString<Char>> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericStringView<Char>&&, GenericString<Char>> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<const GenericStringView<Char>&, GenericString<Char>> { typedef GenericString<Char> _; };

template<typename Char> struct CommonTypeRef<GenericStringView<Char>, GenericString<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericStringView<Char>&, GenericString<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericStringView<Char>&&, GenericString<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<const GenericStringView<Char>&, GenericString<Char>&> { typedef GenericString<Char> _; };

template<typename Char> struct CommonTypeRef<GenericStringView<Char>, GenericString<Char>&&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericStringView<Char>&, GenericString<Char>&&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericStringView<Char>&&, GenericString<Char>&&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<const GenericStringView<Char>&, GenericString<Char>&&> { typedef GenericString<Char> _; };

template<typename Char> struct CommonTypeRef<GenericStringView<Char>, const GenericString<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericStringView<Char>&, const GenericString<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericStringView<Char>&&, const GenericString<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<const GenericStringView<Char>&, const GenericString<Char>&> { typedef GenericString<Char> _; };



template<typename Char> struct CommonTypeRef<GenericString<Char>, GenericStringView<Char>> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericString<Char>, GenericStringView<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericString<Char>, GenericStringView<Char>&&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericString<Char>, const GenericStringView<Char>&> { typedef GenericString<Char> _; };

template<typename Char> struct CommonTypeRef<GenericString<Char>&, GenericStringView<Char>> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericString<Char>&, GenericStringView<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericString<Char>&, GenericStringView<Char>&&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericString<Char>&, const GenericStringView<Char>&> { typedef GenericString<Char> _; };

template<typename Char> struct CommonTypeRef<GenericString<Char>&&, GenericStringView<Char>> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericString<Char>&&, GenericStringView<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericString<Char>&&, GenericStringView<Char>&&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericString<Char>&&, const GenericStringView<Char>&> { typedef GenericString<Char> _; };

template<typename Char> struct CommonTypeRef<const GenericString<Char>&, GenericStringView<Char>> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<const GenericString<Char>&, GenericStringView<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<const GenericString<Char>&, GenericStringView<Char>&&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<const GenericString<Char>&, const GenericStringView<Char>&> { typedef GenericString<Char> _; };



template<typename Char> struct CommonTypeRef<GenericStringView<const Char>, GenericString<Char>> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericStringView<const Char>&, GenericString<Char>> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericStringView<const Char>&&, GenericString<Char>> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<const GenericStringView<const Char>&, GenericString<Char>> { typedef GenericString<Char> _; };

template<typename Char> struct CommonTypeRef<GenericStringView<const Char>, GenericString<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericStringView<const Char>&, GenericString<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericStringView<const Char>&&, GenericString<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<const GenericStringView<const Char>&, GenericString<Char>&> { typedef GenericString<Char> _; };

template<typename Char> struct CommonTypeRef<GenericStringView<const Char>, GenericString<Char>&&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericStringView<const Char>&, GenericString<Char>&&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericStringView<const Char>&&, GenericString<Char>&&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<const GenericStringView<const Char>&, GenericString<Char>&&> { typedef GenericString<Char> _; };

template<typename Char> struct CommonTypeRef<GenericStringView<const Char>, const GenericString<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericStringView<const Char>&, const GenericString<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericStringView<const Char>&&, const GenericString<Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<const GenericStringView<const Char>&, const GenericString<Char>&> { typedef GenericString<Char> _; };



template<typename Char> struct CommonTypeRef<GenericString<Char>, GenericStringView<const Char>> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericString<Char>, GenericStringView<const Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericString<Char>, GenericStringView<const Char>&&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericString<Char>, const GenericStringView<const Char>&> { typedef GenericString<Char> _; };

template<typename Char> struct CommonTypeRef<GenericString<Char>&, GenericStringView<const Char>> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericString<Char>&, GenericStringView<const Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericString<Char>&, GenericStringView<const Char>&&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericString<Char>&, const GenericStringView<const Char>&> { typedef GenericString<Char> _; };

template<typename Char> struct CommonTypeRef<GenericString<Char>&&, GenericStringView<const Char>> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericString<Char>&&, GenericStringView<const Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericString<Char>&&, GenericStringView<const Char>&&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<GenericString<Char>&&, const GenericStringView<const Char>&> { typedef GenericString<Char> _; };

template<typename Char> struct CommonTypeRef<const GenericString<Char>&, GenericStringView<const Char>> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<const GenericString<Char>&, GenericStringView<const Char>&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<const GenericString<Char>&, GenericStringView<const Char>&&> { typedef GenericString<Char> _; };
template<typename Char> struct CommonTypeRef<const GenericString<Char>&, const GenericStringView<const Char>&> { typedef GenericString<Char> _; };

}


static_assert(TypeEquals<CommonTypeRef<String, StringView>, String>::_, "ERROR!");
static_assert(Container::IsSequentialContainer<String>::_, "ERROR!");
static_assert(Container::IsDynamicArrayContainer<String>::_, "ERROR!");
static_assert(Meta::IsTriviallySerializable<Range::ValueTypeOf<String>>::_, "ERROR!");

}

template<typename Char> GenericString<Char> operator+(GenericStringView<const Char> lhs, GenericStringView<const Char> rhs)
{
	GenericString<Char> result;
	result.Reserve(lhs.Length()+rhs.Length());
	result += lhs;
	result += rhs;
	return result;
}

template<typename Char> GenericString<Char> operator+(GenericStringView<const Char> lhs, Char rhs)
{
	GenericString<Char> result;
	result.Reserve(lhs.Length()+1);
	result += lhs;
	result += rhs;
	return result;
}

template<typename Char> GenericString<Char> operator+(Char lhs, GenericStringView<const Char> rhs)
{
	GenericString<Char> result;
	result.Reserve(1+rhs.Length());
	result += lhs;
	result += rhs;
	return result;
}

template<typename S1, typename S2, typename Char=Range::ValueTypeOfAs<S1>> forceinline Meta::EnableIf<
	Range::IsAsArrayRange<S1>::_ && Range::IsAsArrayRange<S2>::_ &&
	(Range::HasData<S1>::_ || Range::HasData<S2>::_ || Range::IsInputRange<S1>::_ || Range::IsInputRange<S2>::_) && //Чтобы не конфликтовать с оператором из STL
	Meta::TypeEquals<Char, Range::ValueTypeOfAs<S2>>::_,
GenericString<Char>> operator+(S1&& lhs, S2&& rhs)
{return GenericStringView<const Char>(lhs)+GenericStringView<const Char>(rhs);}

template<typename S1, typename S2, typename Char=Range::ValueTypeOfAs<S1>> forceinline Meta::EnableIf<
	Range::IsAsConsumableRange<S1>::_ && Range::IsAsConsumableRange<S2>::_ &&
	(!Range::IsAsArrayRange<S1>::_ || !Range::IsAsArrayRange<S2>::_ ||
	!Meta::TypeEquals<Char, Range::ValueTypeOfAs<S2>>::_),
GenericString<Char>> operator+(S1&& lhs, S2&& rhs)
{
	GenericString<Char> result;
	result.Reserve(Range::LengthOr0(lhs)+Range::LengthOr0(rhs));
	Algo::CopyTo(Range::Forward<S1>(lhs), Range::LastAppender(result));
	Algo::CopyTo(Range::Forward<S2>(rhs), Range::LastAppender(result));
	return result;
}

template<typename R, typename Char, typename Char2=Range::ValueTypeOfAs<R>> Meta::EnableIf<
	Range::IsAsConsumableRange<R>::_ &&
	(!Container::Has_data<R>::_ || Range::HasAsRangeMethod<R>::_ || Range::IsInputRange<R>::_) && //Чтобы не конфликтовать с STL
	Meta::IsCharType<Char2>::_ && (Meta::IsCharType<Char>::_ || Meta::IsIntegralType<Char>::_),
GenericString<Char2>> operator+(R&& lhs, Char rhs)
{
	GenericString<Char2> result;
	result.Reserve(Range::LengthOr0(lhs)+1);
	result += Range::Forward<R>(lhs);
	result += rhs;
	return result;
}

template<typename R, typename Char, typename Char2=Range::ValueTypeOfAs<R>> Meta::EnableIf<
	Range::IsAsConsumableRange<R>::_ &&
	(!Container::Has_data<R>::_ || Range::HasAsRangeMethod<R>::_ || Range::IsInputRange<R>::_) && //Чтобы не конфликтовать с STL
	Meta::IsCharType<Char2>::_ && (Meta::IsCharType<Char>::_ || Meta::IsIntegralType<Char>::_),
GenericString<Char2>> operator+(Char lhs, R&& rhs)
{
	GenericString<Char2> result;
	result.Reserve(1+Range::LengthOr0(rhs));
	result += lhs;
	result += Range::Forward<R>(rhs);
	return result;
}

}

INTRA_WARNING_POP
