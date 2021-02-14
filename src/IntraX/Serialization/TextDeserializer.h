﻿#pragma once

#include "Intra/Core.h"
#include "Intra/Container/Tuple.h"
#include "Intra/EachField.h"

#include "Intra/Range/StringView.h"
#include "IntraX/Utils/AsciiSet.h"

#include "Intra/CContainer.h"

#include "Intra/Range/Search/RecursiveBlock.h"
#include "Intra/Range/Comparison.h"
#include "Intra/Range/TakeUntil.h"
#include "Intra/Range/TakeUntilAny.h"
#include "Intra/Range/Stream/Parse.h"
#include "Intra/Range/Map.h"

#include "Intra/Reflection.h"
#include "LanguageParams.h"

#include "IntraX/Container/Sequential/String.h"
#include "IntraX/Container/Operations.hh"


namespace Intra { INTRA_BEGIN

template<typename I> struct GenericTextDeserializer;

template<typename I> struct GenericTextDeserializerStructVisitor
{
	GenericTextDeserializer<I>* Me;
	Span<const const char*> FieldNames;
	bool Began;
	TextSerializerParams::TypeFlags Type;

	template<typename T> GenericTextDeserializerStructVisitor& operator()(T& t);
};

template<typename I> struct GenericTextDeserializer
{
	GenericTextDeserializer(const LanguageParams& langParams, const I& input):
		Lang(langParams), NestingLevel(0), Line(0), Input(input), Log() {}


	bool IgnoreField()
	{
		int counter = 1;
		TakeRecursiveBlockAdvance(Input, counter, nullptr,
			Lang.StructInstanceOpen, Lang.StructInstanceClose, Lang.FieldSeparator,
		Span<const Tuple<StringView, StringView>>{
			{Lang.MultiLineCommentBegin, Lang.MultiLineCommentEnd},
			{Lang.OneLineCommentBegin, "\n"},
			{Lang.MultiLineStringBegin, Lang.MultiLineStringEnd},
			{Lang.ArrayOpen, Lang.ArrayClose}
		},
		Span<const Tuple<StringView, StringView>>{});
		INTRA_POSTCONDITION(counter == 0 || counter == 1);
		return counter == 0;
	}

	bool IgnoreArrayElement()
	{
		int counter = 1;
		auto inputCopy = Input;
		size_t charsRead = 0;
		TakeRecursiveBlockAdvance(Input, counter, &charsRead,
			StringView(), Lang.ArrayClose, Lang.ArrayElementSeparator,
		Span<const Tuple<StringView, StringView>>{
			{Lang.MultiLineCommentBegin, Lang.MultiLineCommentEnd},
			{Lang.OneLineCommentBegin, StringView("\n")},
			{Lang.MultiLineStringBegin, Lang.MultiLineStringEnd}
		},
		Span<const Tuple<StringView, StringView>>{
			{Lang.StructInstanceOpen, Lang.StructInstanceClose},
			{Lang.ArrayOpen, Lang.ArrayClose}
		});
		if(counter == 0)
		{
			Input = Move(inputCopy);
			PopFirstExactly(Input, charsRead-Lang.ArrayClose.Length());
			return true;
		}
		INTRA_POSTCONDITION(counter == 1);
		return false;
	}

	bool NextField(TextSerializerParams::TypeFlags typeFlag)
	{
		if(typeFlag & TextSerializerParams::TypeFlags_Struct)
		{
			INTRA_DEBUG_ASSERT(typeFlag == TextSerializerParams::TypeFlags_Struct);
			if(!Expect(Lang.FieldSeparator))
			{
				if(!Input.Empty()) IgnoreField();
				return false;
			}
		}
		if(typeFlag & TextSerializerParams::TypeFlags_Tuple)
		{
			INTRA_DEBUG_ASSERT(typeFlag == TextSerializerParams::TypeFlags_Tuple);
			if(!Expect(Lang.TupleFieldSeparator))
			{
				if(!Input.Empty()) IgnoreField();
				return false;
			}
		}
		if(typeFlag & TextSerializerParams::TypeFlags_Array)
		{
			INTRA_DEBUG_ASSERT(typeFlag == TextSerializerParams::TypeFlags_Array);
			if(!Expect(Lang.ArrayElementSeparator))
				return !Input.Empty() && !IgnoreArrayElement();
		}
		if(typeFlag & TextSerializerParams::TypeFlags_StructArray)
		{
			INTRA_DEBUG_ASSERT(typeFlag == TextSerializerParams::TypeFlags_StructArray);
			if(!Expect(Lang.ArrayElementSeparator))
				return !Input.Empty() && !IgnoreArrayElement();
		}
		SkipAllSpacesAndComments();
		return true;
	}

	TTakeResult<I> ParseIdentifier()
	{
		static const AsciiSet isNotIdentifierFirstChar = AsciiSets.NotIdentifierChars | AsciiSets.Digits;
		return ParseIdentifierAdvance(Input,
			isNotIdentifierFirstChar, AsciiSets.NotIdentifierChars);
	}

	/// Прочитать имя поля и пропустить оператор присваивания, перейдя к правой части присваивания.
	/// Если в процессе обнаруживается ошибка, функция возвращает на исходную позицию и возвращает nullptr.
	/// Иначе возвращает прочитанное имя поля.
	StringView FieldAssignmentBeginning()
	{
		auto oldPos = Input;
		auto oldLine = Line;
		StringView name;
		SkipAllSpacesAndComments();
		if(!ExpectAdvance(Input, Lang.LeftFieldNameBeginQuote))
			goto error;
		SkipAllSpacesAndComments();
		name = ParseIdentifier();
		SkipAllSpacesAndComments();
		if(!ExpectAdvance(Input, Lang.LeftFieldNameEndQuote))
			goto error;
		SkipAllSpacesAndComments();
		if(!ExpectAdvance(Input, Lang.LeftAssignmentOperator))
			goto error;
		return name;

	error:
		Input = oldPos;
		Line = oldLine;
		return nullptr;
	}

	StringView FieldAssignmentEnding()
	{
		const auto oldPos = Input;
		const size_t oldLine = Line;
		StringView name;
		SkipAllSpacesAndComments();
		if(!ExpectAdvance(Input, Lang.RightAssignmentOperator))
			goto error;
		SkipAllSpacesAndComments();
		if(!ExpectAdvance(Input, Lang.RightFieldNameBeginQuote))
			goto error;
		SkipAllSpacesAndComments();
		name = ParseIdentifier();
		SkipAllSpacesAndComments();
		if(!ExpectAdvance(Input, Lang.RightFieldNameEndQuote))
			goto error;
		return name;

	error:
		Input = oldPos;
		Line = oldLine;
		return nullptr;
	}

	void StructInstanceDefinitionBegin(TextSerializerParams::TypeFlags type)
	{
		const StringView openStr = (type & TextSerializerParams::TypeFlags_Struct)? Lang.StructInstanceOpen: Lang.TupleOpen;
		if(Expect(openStr))
		{
			NestingLevel++;
			return;
		}

		if(Input.Empty()) return;
		int counter = -1;
		TakeRecursiveBlockAdvance(Input, counter, nullptr,
			Lang.StructInstanceOpen, StringView(), StringView(),
			Span<const Tuple<StringView, StringView>>{
				{Lang.MultiLineCommentBegin, Lang.MultiLineCommentEnd},
				{Lang.OneLineCommentBegin, "\n"},
				{Lang.MultiLineStringBegin, Lang.MultiLineStringEnd},
				{Lang.StringQuote, Lang.StringQuote}
			},
			Span<const Tuple<StringView, StringView>>{
				{Lang.ArrayOpen, Lang.ArrayClose}
			}
		);
		NestingLevel++;
	}

	void StructInstanceDefinitionEnd(TextSerializerParams::TypeFlags type)
	{
		NestingLevel--;
		const StringView closeStr = (type & TextSerializerParams::TypeFlags_Struct)? Lang.StructInstanceClose: Lang.TupleClose;
		if(Expect(closeStr)) return;
		int counter = 1;
		TakeRecursiveBlockAdvance(Input, counter, nullptr,
			Lang.StructInstanceOpen, Lang.StructInstanceClose, StringView(),
			Span<const Tuple<StringView, StringView>>{
				{Lang.MultiLineCommentBegin, Lang.MultiLineCommentEnd},
				{Lang.OneLineCommentBegin, "\n"},
				{Lang.MultiLineStringBegin, Lang.MultiLineStringEnd},
				{Lang.StringQuote, Lang.StringQuote}
			},
			Span<const Tuple<StringView, StringView>>{
				{Lang.ArrayOpen, Lang.ArrayClose}
			}
		);
	}

	INTRA_FORCEINLINE void SkipAllSpaces()
	{Line += SkipSpacesCountLinesAdvance(Input);}

	void SkipAllSpacesAndComments()
	{
		for(;;)
		{
			SkipAllSpaces();
			if(!Lang.OneLineCommentBegin.Empty() && ExpectAdvance(Input, Lang.OneLineCommentBegin))
			{
				int counter = 1;
				StringView commentBlock = TakeRecursiveBlockAdvance(Input, counter, nullptr,
					Lang.OneLineCommentBegin, "\n", StringView());
				Line += CountLinesAdvance(commentBlock);
				continue;
			}
			if(!Lang.MultiLineCommentBegin.Empty() && ExpectAdvance(Input, Lang.MultiLineCommentBegin))
			{
				int counter = 1;
				StringView commentBlock = TakeRecursiveBlockAdvance(Input, counter, nullptr,
					Lang.MultiLineCommentBegin, Lang.MultiLineCommentEnd, StringView());
				Line += CountLinesAdvance(commentBlock);
				continue;
			}
			break;
		}
	}
	
	bool Expect(StringView stringToExpect)
	{
		SkipAllSpacesAndComments();
		if(ExpectAdvance(Input, stringToExpect)) return true;
		if(Input.Empty())
		{
			static const StringView eofStr = "Unexpected EOF!\r\n";
			if(!EndsWith(Log, eofStr)) Log += eofStr;
			return false;
		}
		LogExpectError(TakeUntil(Input, AsciiSets.Spaces), {stringToExpect});
		return false;
	}

	index_t ExpectOneOf(Span<const StringView> stringsToExpect)
	{
		SkipAllSpaces();
		size_t maxStrLength=0;
		for(size_t i=0; i<stringsToExpect.Length(); i++)
		{
			if(ExpectAdvance(Input, stringsToExpect[i])) return index_t(i);
			if(maxStrLength<stringsToExpect[i].Length())
				maxStrLength = stringsToExpect[i].Length();
		}
		LogExpectError(TakeUntil(Input, AsciiSets.Spaces), stringsToExpect);
		return -1;
	}

	void LogExpectError(StringView token, Span<const StringView> expected)
	{
		char buf[1024];
		SpanOutput<char> dst = buf;
		dst << "Error(" << Line << "): token \"" << token << "\", where expected ";
		if(expected.Length()>1) dst << "one of the folowing tokens: " << StringOf(expected, ", ", "\"", "\"");
		if(expected.Length()==1) dst << "token \"" << expected[0] << "\"";
		dst << ".\r\n";
		Log += dst.WrittenRange();
	}


	/// Десериализовать структуру со статической информацией о полях
	template<typename T> void DeserializeStruct(T& dst, Span<const const char*> fieldNames)
	{
		StructInstanceDefinitionBegin(TextSerializerParams::TypeFlags_Struct);
		GenericTextDeserializerStructVisitor<I> visitor{this, fieldNames, false, TextSerializerParams::TypeFlags_Struct};
		if(fieldNames.Empty())
		{
			ForEachField(dst, visitor);
			StructInstanceDefinitionEnd(TextSerializerParams::TypeFlags_Struct);
			return;
		}

		bool expectSeparator = false;
		for(size_t i=0; ; i++)
		{
			SkipAllSpacesAndComments();
			if(StartsWith(Input, Lang.StructInstanceClose)) break;

			if(expectSeparator)
			{
				if(!NextField(TextSerializerParams::TypeFlags_Struct)) break;
			}
			expectSeparator = true;

			StringView name = FieldAssignmentBeginning();

			//TODO: учесть, что имя поля может идти после его значения
			size_t index = 0;
			if(name.Empty()) index = fieldNames.Length();
			else index = CountUntil(Map(fieldNames, FCastTo<StringView>), name);
			if(index == fieldNames.Length() && name != nullptr) //Если такого поля в структуре нет, пропускаем его
			{
				const bool finished = IgnoreField();
				if(!finished)
				{
					expectSeparator = false;
					continue;
				}
				NestingLevel--;
				return;
			}
			if(index != fieldNames.Length() || name == nullptr)
				dst.VisitFieldById(name != nullptr? index: i, *this);

			if(Lang.AddFieldNameAfterAssignment)
			{
				StringView name2 = FieldAssignmentEnding();
				if(name != nullptr && name2 != nullptr && name != name2) LogExpectError(name2, {name});
			}
		}
		StructInstanceDefinitionEnd(TextSerializerParams::TypeFlags_Struct);
	}

	/// Десериализовать структуру со статической информацией о полях
	template<typename T> void DeserializeStruct(T& dst)
	{DeserializeStruct(dst, T::ReflectionFieldNames());}

	/// Десериализовать структуру со статической информацией о полях
	template<typename T> void DeserializeTuple(T& dst)
	{
		StructInstanceDefinitionBegin(TextSerializerParams::TypeFlags_Tuple);
		GenericTextDeserializerStructVisitor<I> visitor{this, nullptr, false, TextSerializerParams::TypeFlags_Tuple};
		ForEachField(dst, visitor);
		StructInstanceDefinitionEnd(TextSerializerParams::TypeFlags_Tuple);
	}

	void ConsumeArrayOpen()
	{
		if(Expect(Lang.ArrayOpen)) return;
		
		//Произошла ошибка, о которой мы уже сообщили в лог.
		//Пытаемся восстановиться. Для этого ищем начало массива на текущем уровне вложенности.
		int counter = -1;
		TakeRecursiveBlockAdvance(Input, counter, nullptr,
			Lang.ArrayOpen, StringView(), StringView(),
			Span<const Tuple<StringView, StringView>>{
				{Lang.MultiLineCommentBegin, Lang.MultiLineCommentEnd},
				{Lang.OneLineCommentBegin, "\n"},
				{Lang.MultiLineStringBegin, Lang.MultiLineStringEnd},
				{Lang.StringQuote, Lang.StringQuote}
			},
			Span<const Tuple<StringView, StringView>>{
				{Lang.StructInstanceOpen, Lang.StructInstanceClose}
			}
		);
	}

	void ConsumeArrayClose()
	{
		if(Expect(Lang.ArrayClose)) return;

		//Произошла ошибка, о которой мы уже сообщили в лог.
		//Пытаемся восстановиться. Для этого ищем конец массива соответствующий текущему уровню вложенности.
		int counter = 1;
		TakeRecursiveBlockAdvance(Input, counter, nullptr,
			Lang.ArrayOpen, Lang.ArrayClose, StringView(),
			Span<const Tuple<StringView, StringView>>{
				{Lang.MultiLineCommentBegin, Lang.MultiLineCommentEnd},
				{Lang.OneLineCommentBegin, "\n"},
				{Lang.MultiLineStringBegin, Lang.MultiLineStringEnd},
				{Lang.StringQuote, Lang.StringQuote}
			},
			Span<const Tuple<StringView, StringView>>{
				{Lang.StructInstanceOpen, Lang.StructInstanceClose}
			}
		);
	}

	/// Десериализовать элементы в output-диапазон
	template<typename R> void DeserializeRange(R& outputRange)
	{
		typedef TRangeValue<R> T;

		ConsumeArrayOpen();

		SkipAllSpacesAndComments();
		if( (Lang.ArrayClose.Empty() && !StartsWith(Input, Lang.RightFieldNameBeginQuote)) ||
			!StartsWith(Input, Lang.ArrayClose) )
		{
			outputRange.Put(Deserialize<T>());
			SkipAllSpacesAndComments();
			while( (Lang.ArrayClose.Empty() && !StartsWith(Input, Lang.RightFieldNameBeginQuote)) ||
				!StartsWith(Input, Lang.ArrayClose) )
			{
				if(NextField(TextSerializerParams::TypeFlags_Array))
					outputRange.Put(Deserialize<T>());
				else if(Input.Empty()) return;
				SkipAllSpacesAndComments();
			}
		}

		ConsumeArrayClose();
	}


	/// Десериализация вещественных чисел
	template<typename T> INTRA_FORCEINLINE Requires<
		CBasicFloatingPoint<T>,
	GenericTextDeserializer&> operator>>(T& v)
	{
		v = ParseAdvance<T>(Input, Lang.DecimalSeparator);
		return *this;
	}

	/// Десериализация целых чисел
	template<typename T> INTRA_FORCEINLINE Requires<
		CBasicIntegral<T>,
	GenericTextDeserializer&> operator>>(T& v)
	{
		Input >> v;
		return *this;
	}

	/// Десериализация bool
	INTRA_FORCEINLINE GenericTextDeserializer& operator>>(bool& v)
	{
		SkipAllSpaces();
		const StringView identifier = ParseIdentifier();
		if(identifier == Lang.FalseTrueNames[true])
		{
			v = true;
			return *this;
		}
		if(identifier != Lang.FalseTrueNames[false])
			LogExpectError(identifier, Lang.FalseTrueNames);
		v = false;
		return *this;
	}

	/// Десериализация кортежей
	template<typename T> INTRA_FORCEINLINE Requires<
		CHasForEachField<T> &&
		!CHasReflectionFieldNamesMethod<T>,
	GenericTextDeserializer&> operator>>(T& v)
	{
		DeserializeTuple(v);
		return *this;
	}

	/// Десериализация структур
	template<typename T> INTRA_FORCEINLINE Requires<
		CHasForEachField<T> &&
		CHasReflectionFieldNamesMethod<T>,
	GenericTextDeserializer&> operator>>(T& v)
	{
		DeserializeStruct(v);
		return *this;
	}

	template<typename C> Requires<
		CCharList<C> &&
		CHas_resize<C>,
	GenericTextDeserializer&> operator>>(C& dst)
	{
		if(!Expect(Lang.StringQuote)) return *this;

		auto escapedResult = TakeUntilAdvance(Input, Lang.StringQuote);
		PopFirstExactly(Input, Lang.StringQuote.Length());

		/*dst.SetLengthUninitialized(StringMultiReplaceAsciiLength(escapedResult, {"\\n", "\\r", "\\t"}, {"\n", "\r", "\t"}));
		auto dstRange = dst.AsRange();
		StringMultiReplaceAscii(escapedResult, dstRange, {"\\n", "\\r", "\\t"}, {"\n", "\r", "\t"});*/


		//TODO: Вынести экранирование строк в LanguageParams и добавить экранирование кавычек
		const Tuple<StringView, StringView> replacements[] = {{"\\n", "\n"}, {"\\r", "\r"}, {"\\t", "\t"}};
		CountRange<char> counter;
		MultiReplaceToAdvance(escapedResult, counter, replacements);
		SetCountTryNotInit(dst, counter.Counter);
		auto strData = RangeOf(dst);
		MultiReplaceToAdvance(escapedResult, strData, replacements);

		return *this;
	}

	/// Десериализовать массив
	template<typename C> Requires<
		CHas_clear<C> &&
		CHas_push_back<C> &&
		!CChar<TRangeValue<C>>,
	GenericTextDeserializer&> operator>>(C& container)
	{
		container.clear();
		auto appender = LastAppender(container);
		DeserializeRange(appender);
		return *this;
	}

	/// Десериализовать диапазон
	template<typename R> INTRA_FORCEINLINE Requires<
		COutput<R>,
	GenericTextDeserializer&> operator>>(R&& range)
	{
		DeserializeRange(range);
		return *this;
	}

	/// Десериализовать массив
	template<typename T, size_t N> INTRA_FORCEINLINE GenericTextDeserializer& operator>>(T(&arr)[N])
	{
		Span<T> range = arr;
		DeserializeRange(range);
		return *this;
	}


	/// Десериализовать любое значение
	template<typename T> INTRA_FORCEINLINE Requires<
		!CConst<T>,
	GenericTextDeserializer&> operator()(T&& value) {return *this >> value;}

	/// Десериализовать любое значение
	template<typename T> INTRA_FORCEINLINE T Deserialize()
	{
		T result;
		*this >> result;
		return result;
	}

	void ResetStream(const I& stream)
	{
		Input = stream;
		Line = 0;
		NestingLevel = 0;
	}

	LanguageParams Lang;
	int NestingLevel;
	size_t Line;
	I Input;
	String Log;
};

template<typename I> template<typename T> GenericTextDeserializerStructVisitor<I>&
	GenericTextDeserializerStructVisitor<I>::operator()(T& t)
{
	if(Began)
	{
		Me->NextField(Type);
		if(!FieldNames.Empty()) FieldNames.PopFirst();
	}
	else Began = true;

	if(!FieldNames.Empty())
		Me->FieldAssignmentBeginning();

	Me->operator()(t);

	if(!FieldNames.Empty() && Me->Lang.AddFieldNameAfterAssignment)
		Me->FieldAssignmentEnding();

	return *this;
}

template<typename I> INTRA_FORCEINLINE Requires<
	CForwardList<I>,
GenericTextDeserializer<TRemoveConstRef<I>>> TextDeserializer(const LanguageParams& language, I&& input)
{return {language, ForwardAsRange<I>(input)};}

} INTRA_END