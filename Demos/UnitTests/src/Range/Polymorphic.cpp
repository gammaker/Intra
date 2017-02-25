﻿#include "Platform/CppWarnings.h"

INTRA_DISABLE_REDUNDANT_WARNINGS

#if(defined(_MSC_VER) && !defined(__GNUC__) && !defined(_HAS_EXCEPTIONS))
#define _HAS_EXCEPTIONS 0
#endif

#include "Header.h"
#include "IO/Stream.h"
#include "Range/Stream.h"
#include "Algo/Reduction.h"
#include "Range/Generators/ArrayRange.h"
#include "Range.hh"
#include "Math/MathRanges.h"
#include "Math/Random.h"
#include "Container/Sequential/List.h"

#include <stdlib.h>

using namespace Intra;
using namespace Intra::IO;
using namespace Intra::Range;
using namespace Intra::Algo;

template<typename T> static void PrintPolymorphicRange(IO::IFormattedWriter& output, InputRange<T> range)
{
	output.Print("[");
	bool firstIteration = true;
	while(!range.Empty())
	{
		if(!firstIteration) output.Print(", ");
		else firstIteration = false;
		output.Print(range.First());
		range.PopFirst();
	}
	output.PrintLine("]");
}

static int SumPolymorphicRange(InputRange<int> ints)
{
	int sum = 0;
	while(!ints.Empty())
		sum += ints.GetNext();
	return sum;
}


struct ivec3
{
	int x, y, z;
	template<typename V> void ForEachField(V&& f) const {f(x), f(y), f(z);}
};

static void TestSumRange(IO::IFormattedWriter& output)
{
	int ints[] = {3, 53, 63, 3, 321, 34253, 35434, 2};
	int sum = SumPolymorphicRange(ints);
	output.PrintLine("sum of ", ints, " = ", sum);

	InputRange<const char> myRange = StringView("Диапазон");
	String myRange2Str = "Супер Диапазон";
	//myRange = myRange2Str();
	char c[40];
	auto r = ArrayRange<char>(c);
	r << Meta::Move(myRange);

	ivec3 vectors[] = {{1, 2, 3}, {1, 64, 7}, {43, 5, 342}, {5, 45, 4}};
	RandomAccessRange<ivec3&> vectors1;
	vectors1 = vectors;
	vectors1[1] = {2, 3, 4};
	InputRange<int> xvectors = Map(vectors, [](const ivec3& v) {return v.x;});
	int xsum = SumPolymorphicRange(Meta::Move(xvectors));
	output.PrintLine("x sum of ", vectors, " = ", xsum);
}

void TestComposedPolymorphicRange(IO::IFormattedWriter& output)
{
	auto someRecurrence = Take(Drop(Cycle(Take(Recurrence(
		[](int a, int b) {return a*2+b; }, 1, 1

	), 17)), 3), 22);
	Console.PrintLine("Представляем сложную последовательность в виде полиморфного input-диапазона:");
	InputRange<int> someRecurrencePolymorphic = someRecurrence;
	PrintPolymorphicRange(output, Meta::Move(someRecurrencePolymorphic));

	Console.PrintLine("Полиморфный диапазон seq содержит генератор 100 случайных чисел от 0 до 999 с отбором квадратов тех из них, которые делятся на 7: ");
	InputRange<uint> seq = Map(
		Filter(
			Take(Generate([]() {return Math::Random<uint>::Global(1000); }), 500),
			[](uint x) {return x%7==0; }),
		Math::Sqr<uint>);
	PrintPolymorphicRange(output, Meta::Move(seq));

	Console.PrintLine(endl, "Присвоили той же переменной seq диапазон другого типа и выведем его снова:");
	seq = Take(Generate(rand), 50);
	PrintPolymorphicRange(output, Meta::Move(seq));
}

void TestPolymorphicRange(IO::IFormattedWriter& output)
{
	TestSumRange(output);
	TestComposedPolymorphicRange(output);
}