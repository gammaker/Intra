﻿#include "PerfTestSort.h"
#include "Test/UnitTest.h"
#include "Test/PerformanceTest.h"
#include "Algorithms/Sort.h"
#include "Core/Time.h"
using namespace Intra;

#include <algorithm>

static const short arrayForSortTesting[] = {2, 4234, -9788, 23, 5, 245, 2, 24, 5, -9890, 2, 5, 4552, 54, 3, -932, 123, 342, 24321, -234};

INTRA_UNITTEST("Sort algorithms unit tests")
{
	Array<short> arrUnsorted = arrayForSortTesting;
	Array<short> arrInsertion = arrUnsorted;
	Array<short> arrShell = arrUnsorted;
	Array<short> arrQuick = arrUnsorted;
	Array<short> arrRadix = arrUnsorted;
	Array<short> arrMerge = arrUnsorted;
	Array<short> arrHeap = arrUnsorted;
	Array<short> arrSelection = arrUnsorted;
	Array<short> arrStdSort = arrUnsorted;

	Algo::InsertionSort(arrInsertion.AsRange());
	Algo::ShellSort(arrShell.AsRange());
	Algo::QuickSort(arrQuick.AsRange());
	Algo::RadixSort(arrRadix.AsRange());
	Algo::MergeSort(arrMerge.AsRange());
	Algo::HeapSort(arrHeap.AsRange());
	Algo::SelectionSort(arrSelection.AsRange());
	std::sort(arrStdSort.begin(), arrStdSort.end());

	INTRA_TEST_ASSERT_EQUALS(arrInsertion, arrStdSort);
	INTRA_TEST_ASSERT_EQUALS(arrShell, arrStdSort);
	INTRA_TEST_ASSERT_EQUALS(arrQuick, arrStdSort);
	INTRA_TEST_ASSERT_EQUALS(arrRadix, arrStdSort);
	INTRA_TEST_ASSERT_EQUALS(arrMerge, arrStdSort);
	INTRA_TEST_ASSERT_EQUALS(arrHeap, arrStdSort);
	INTRA_TEST_ASSERT_EQUALS(arrSelection, arrStdSort);
};

template<typename T, typename Comparer = Algo::Comparers::Function<T>> double TestInsertionSorting(size_t size, Comparer comparer = Op::Less<T>)
{
	Array<T> arr = GetRandomValueArray<T>(size);
	Timer tim;
	Algo::InsertionSort(arr.AsRange(), comparer);
	double result = tim.GetTime();
	INTRA_ASSERT(arr().IsSorted());
	return result;
}

template<typename T, typename Comparer = Algo::Comparers::Function<T>> double TestShellSorting(size_t size, Comparer comparer = Op::Less<T>)
{
	Array<T> arr = GetRandomValueArray<T>(size);
	Timer tim;
	Algo::ShellSort(arr.AsRange(), comparer);
	double result = tim.GetTime();
	INTRA_ASSERT(arr().IsSorted());
	return result;
}

template<typename T, typename Comparer = Algo::Comparers::Function<T>> double TestQuickSorting(size_t size, Comparer comparer = Op::Less<T>)
{
	Array<T> arr = GetRandomValueArray<T>(size);
	Timer tim;
	Algo::QuickSort(arr.AsRange(), comparer);
	double result = tim.GetTime();
	INTRA_ASSERT(arr().IsSorted());
	return result;
}

template<typename T> double TestRadixSorting(size_t size)
{
	Array<T> arr = GetRandomValueArray<T>(size);
	Timer tim;
	Algo::RadixSort(arr.AsRange());
	double result = tim.GetTime();
	INTRA_ASSERT(arr().IsSorted());
	return result;
}

template<typename T, typename Comparer = Algo::Comparers::Function<T>> double TestMergeSorting(size_t size, Comparer comparer = Op::Less<T>)
{
	Array<T> arr = GetRandomValueArray<T>(size);
	Timer tim;
	Algo::MergeSort(arr.AsRange(), comparer);
	double result = tim.GetTime();
	INTRA_ASSERT(arr().IsSorted());
	return result;
}

template<typename T, typename Comparer = Algo::Comparers::Function<T>> double TestSelectionSorting(size_t size, Comparer comparer = Op::Less<T>)
{
	Array<T> arr = GetRandomValueArray<T>(size);
	Timer tim;
	Algo::SelectionSort(arr.AsRange(), comparer);
	double result = tim.GetTime();
	INTRA_ASSERT(arr().IsSorted());
	return result;
}

template<typename T, typename Comparer = Algo::Comparers::Function<T>> double TestHeapSorting(size_t size, Comparer comparer = Op::Less<T>)
{
	Array<T> arr = GetRandomValueArray<T>(size);
	Timer tim;
	Algo::HeapSort(arr.AsRange(), comparer);
	double result = tim.GetTime();
	INTRA_ASSERT(arr().IsSorted());
	return result;
}

template<typename T, typename Comparer = Algo::Comparers::Function<T>> double TestStdSorting(size_t size, Comparer comparer = Op::Less<T>)
{
	Array<T> arr = GetRandomValueArray<T>(size);
	Timer tim;
	std::sort(arr.begin(), arr.end(), comparer);
	double result = tim.GetTime();
	INTRA_ASSERT(arr().IsSorted());
	return result;
}

static const StringView comparedSorts[] = {"std::sort", "InsertionSort", "ShellSort", "QuickSort", "MergeSort", "SelectionSort", "HeapSort", "RadixSort"};
static const StringView comparedSortsWithoutSlow[] = {"std::sort", "ShellSort", "QuickSort", "MergeSort", "HeapSort", "RadixSort"};

template<typename T> void TestAndPrintIntegralTypeSorts(IO::Logger& logger, StringView typeName)
{
	PrintPerformanceResults(logger, "Размер массива " + typeName + ": 100",
		comparedSorts, {TestStdSorting<T>(100)},
		{
			TestInsertionSorting<T>(100),
			TestShellSorting<T>(100),
			TestQuickSorting<T>(100),
			TestMergeSorting<T>(100),
			TestSelectionSorting<T>(100),
			TestHeapSorting<T>(100),
			TestRadixSorting<T>(100)
		});

	PrintPerformanceResults(logger, "Размер массива " + typeName + ": 1000",
		comparedSorts, {TestStdSorting<T>(1000)},
		{
			TestInsertionSorting<T>(1000),
			TestShellSorting<T>(1000),
			TestQuickSorting<T>(1000),
			TestMergeSorting<T>(1000),
			TestSelectionSorting<T>(1000),
			TestHeapSorting<T>(1000),
			TestRadixSorting<T>(1000)
		});

	PrintPerformanceResults(logger, "Размер массива " + typeName + ": 10000",
		comparedSorts, {TestStdSorting<T>(10000)},
		{
			TestInsertionSorting<T>(10000),
			TestShellSorting<T>(10000),
			TestQuickSorting<T>(10000),
			TestMergeSorting<T>(10000),
			TestSelectionSorting<T>(10000),
			TestHeapSorting<T>(10000),
			TestRadixSorting<T>(10000)
		});

	PrintPerformanceResults(logger, "Размер массива " + typeName + ": 100000",
		comparedSorts, {TestStdSorting<T>(100000)},
		{
			TestInsertionSorting<T>(100000),
			TestShellSorting<T>(100000),
			TestQuickSorting<T>(100000),
			TestMergeSorting<T>(100000),
			TestSelectionSorting<T>(100000),
			TestHeapSorting<T>(100000),
			TestRadixSorting<T>(100000)
		});

	PrintPerformanceResults(logger, "Размер массива " + typeName + ": 1000000",
		comparedSortsWithoutSlow, {TestStdSorting<T>(1000000)},
		{
			TestShellSorting<T>(1000000),
			TestQuickSorting<T>(1000000),
			TestMergeSorting<T>(1000000),
			TestHeapSorting<T>(1000000),
			TestRadixSorting<T>(1000000)
		});

	PrintPerformanceResults(logger, "Размер массива " + typeName + ": 10000000",
		comparedSortsWithoutSlow, {TestStdSorting<T>(10000000)},
		{
			TestShellSorting<T>(10000000),
			TestQuickSorting<T>(10000000),
			TestMergeSorting<T>(10000000),
			TestHeapSorting<T>(10000000),
			TestRadixSorting<T>(10000000)
		});
}

void RunSortPerfTests(Intra::IO::Logger& logger)
{
	if(TestGroup gr{logger, "Сортировка случайных массивов short"})
		TestAndPrintIntegralTypeSorts<short>(logger, "short");

	if(TestGroup gr{logger, "Сортировка случайных массивов int"})
		TestAndPrintIntegralTypeSorts<int>(logger, "int");

	if(TestGroup gr{logger, "Сортировка случайных массивов uint"})
		TestAndPrintIntegralTypeSorts<uint>(logger, "uint");

	if(TestGroup gr{logger, "Сортировка случайных массивов long64"})
		TestAndPrintIntegralTypeSorts<long64>(logger, "long64");
}
