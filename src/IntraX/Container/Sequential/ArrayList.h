﻿#pragma once

#include "Intra/Core.h"
#include "Intra/Concepts.h"
#include "Intra/Range/Concepts.h"
#include "Intra/Range/Operations.h"
#include "Intra/Container/Array.h"

#include "Intra/CContainer.h"
#include "Intra/Range/Span.h"

#include "Intra/Range/Comparison.h"
#include "Intra/Range/Search/Single.h"
#include "Intra/Range/Mutation/Copy.h"
#include "IntraX/Memory/Memory.h"
#include "IntraX/Memory/Allocator/Global.h"
#include "IntraX/Container/ForwardDecls.h"
#include "IntraX/Container/Operations.hh"


namespace Intra { INTRA_BEGIN
template<typename T> class ArrayList
{
public:
	constexpr ArrayList() = default;

	explicit INTRA_FORCEINLINE ArrayList(index_t initialLength) {SetCount(initialLength);}

	template<CList R> requires(!CSameUnqualRef<R, ArrayList>)
	ArrayList(R&& values) {AddLastRange(ForwardAsRange<R>(values));}

	constexpr ArrayList(ArrayList&& rhs) noexcept: mBuffer(rhs.mBuffer), mRange(rhs.mRange)
	{
		rhs.mBuffer = nullptr;
		rhs.mRange = nullptr;
	}

	ArrayList(const ArrayList& rhs): ArrayList(CSpanOf(rhs)) {}
	
	~ArrayList() {operator=(nullptr);}


	/** Take ownership of rangeToOwn elements and memory.
	  @warning `rangeToOwn` must be allocated with the same allocator as the template argument Allocator.
	*/
	[[nodiscard]] static ArrayList CreateAsOwnerOf(TUnsafe, Span<T> rangeToOwn)
	{
		ArrayList result;
		result.mRange = result.mBuffer = rangeToOwn;
		return result;
	}

	/** Copy arrays.
	  Destruct all elements of this ArrayList then copy construct all elements from rhs.
	*/
	ArrayList& operator=(const ArrayList& rhs)
	{
		if(this == &rhs) return *this;
		Assign(rhs.AsConstRange());
		return *this;
	}

	/** Move array.
	  Destruct all this ArrayList's elements.
	  Takes ownerwhip of all rhs elements.
	  rhs becomes empty but takes ownership of this ArrayList memory allocation.
	*/
	ArrayList& operator=(ArrayList&& rhs)
	{
		if(this == &rhs) return *this;
		Clear();
		mRange = rhs.mRange;
		rhs.mRange.End = rhs.mRange.Begin;
		Swap(mBuffer, rhs.mBuffer);
		return *this;
	}

	ArrayList& operator=(Span<const T> values)
	{
		INTRA_PRECONDITION(!mBuffer.Overlaps(values));
		Assign(values);
		return *this;
	}

	template<CAccessibleList R> ArrayList& operator=(R&& values) {return operator=(ArrayList(INTRA_FWD(values)));}

	/// Delete all elements and free memory.
	ArrayList& operator=(decltype(nullptr))
	{
		Clear();
		FreeRangeUninitialized(GlobalHeap, mBuffer);
		mBuffer = nullptr;
		mRange = nullptr;
		return *this;
	}

	template<typename U> void Assign(Span<const U> rhs)
	{
		Clear();
		SetLengthUninitialized(rhs.Length());
		CopyInit(mRange, rhs);
	}

	template<typename U> void Assign(Span<U> rhs) {Assign(rhs.AsConstRange());}

	/** Add a new element to the begining of the array by moving `value`.
	  Unlike most other array implementations this operation has O(1) complexity.
	*/
	T& AddFirst(T&& value)
	{
		if(LeftSpace()) return *new(--mRange.Begin) T(INTRA_MOVE(value));
		T temp = INTRA_MOVE(value);
		CheckSpace(0, 1);
		return *new(Construct, --mRange.Begin) T(INTRA_MOVE(temp));
	}

	/** Add \p value to the beginning of the ArrayList.
	  Unlike most other array implementations this operation has amortized constant O(1) complexity.
	*/
	T& AddFirst(const T& value)
	{
		if(LeftSpace()) return *new(--mRange.Begin) T(value);
		T temp = value;
		CheckSpace(0, 1);
		return *new(Construct, --mRange.Begin) T(INTRA_MOVE(temp));
	}

	/** Construct a new element at the beginning of the array with constructor parameters args.
	  Unlike most other array implementations this operation has amortized constant O(1) complexity.
	*/
	template<typename... Args> requires CConstructible<T, Args...>
	T& EmplaceFirst(Args&&... args)
	{
		if(LeftSpace()) return *new(--mRange.Begin) T(INTRA_FWD(args)...);
		T temp(INTRA_FWD(args)...); //if this array contains values referenced by args they will become invalid after reallocation
		CheckSpace(0, 1);
		return *new(Construct, --mRange.Begin) T(INTRA_MOVE(T));
	}

	/** Add all mRange values to the beginning of the array.
	  This operation has linear O(values.Length) complexity and unlike most other array implementations it doesn't depend on this array length.
	*/
	template<CConsumableListOf<T> R> void AddFirstRange(R&& values)
	{
		if constexpr(CForwardRange<R> || CHasLength<R>)
		{
			auto valueRange = ForwardAsRange<R>(values);
			const auto valuesLength = values|Count;
			if(LeftSpace() < valuesLength)
			{
				if(!Empty())
				{
					//use slower implementaation with a temporary copy
					//otherwise reallocation may affect valueRange
					addFirstRangeHelper(INTRA_MOVE(valueRange));
					return;
				}
				Reserve(0, valuesLength);
			}
			mRange.Begin -= valuesLength;
			for(T* dst = mRange.Begin; !valueRange.Empty(); valueRange.PopFirst())
				new(Construct, dst++) T(valueRange.First());
		}
		else addFirstRangeHelper(INTRA_FWD(values));
	}

	/** Add a new element to the end of the array by moving \p value.
	  This operation has amortized constant O(1) complexity.
	*/
	T& AddLast(T&& value)
	{
		if(mRange.End != mBuffer.End) return *new(Construct, mRange.End++) T(INTRA_MOVE(value));
		T temp = INTRA_MOVE(value); //if this array contains value it will become invalid after reallocation, so store it here
		CheckSpace(1, 0);
		return *new(Construct, mRange.End++) T(INTRA_MOVE(temp));
	}

	/** Add \p value to the end of the ArrayList.
	  This operation has amortized constant O(1) complexity.
	*/
	T& AddLast(const T& value)
	{
		if(mRange.End != mBuffer.End) return *new(Construct, mRange.End++) T(value);
		T temp = value; //if this array contains value it will become invalid after reallocation, so store it here
		CheckSpace(1, 0);
		return *new(Construct, mRange.End++) T(INTRA_MOVE(temp));
	}

	/** Construct a new element at the end of the array passing args to constructor.
	  This operation has amortized constant O(1) complexity.
	*/
	template<typename... Args> requires CConstructible<T, Args...>
	T& EmplaceLast(Args&&... args)
	{
		if(RightSpace()) return *new(Construct, mRange.End++) T(INTRA_FWD(args)...);
		T temp(INTRA_FWD(args)...); //if this array contains values referenced by args they will become invalid after reallocation, so construct it before it
		CheckSpace(1, 0);
		return *new(Construct, mRange.End++) T(INTRA_MOVE(temp));
	}

	/** Add all mRange values to the end of the array.
	  This operation has linear O(values.Length) complexity.
	*/
	template<CConsumableListOf<T> R> void AddLastRange(R&& values)
	{
		if constexpr((CForwardRange<AsR> || CHasLength<AsR>))
		{
			auto valueRange = ForwardAsRange<R>(values);
			const auto valuesLength = valueRange|Count;
			if(RightSpace() < valuesLength)
			{
				if(!Empty())
				{
					//use slower implementaation with a temporary copy
					//otherwise reallocation may affect valueRange
					addLastRangeHelper(INTRA_MOVE(valueRange));
					return;
				}
				Reserve(valuesLength, 0);
			}
			for(; !valueRange.Empty(); valueRange.PopFirst())
				new(Construct, mRange.End++) T(valueRange.First());
		}
		else addLastRangeHelper(ForwardAsRange<R>(values));
	}

	/// Set element at index `pos` to `value`.
	/// If `pos` >= `Length()`, it adds `pos` - `Length()` default initialized elements and adds `value`.
	template<typename U> T& Set(Index pos, U&& value)
	{
		if(pos >= Length())
		{
			Reserve(pos + 1);
			SetLength(pos);
			return AddLast(INTRA_FWD(value));
		}
		operator[](pos) = INTRA_FWD(value);
	}
	

	/** Insert ``values`` into ``pos``.
	  As a result for each i-th element its index will stay unchanged if i < ``pos`` and will become i + ``values.Length()`` otherwise.
	  The first inserted element will have index ``pos``.
	*/
	template<typename U> void Insert(Index pos, Span<const U> values)
	{
		if(values.Empty()) return;
		INTRA_PRECONDITION(!mRange.Overlaps(values));
		const auto valuesLength = values.Length();

		//If there is not enough space available, reallocate and move
		if(Length() + valuesLength > Capacity())
		{
			size_t newCapacity = Length() + valuesLength + Capacity()/2;
			Span<T> newBuffer = AllocateRangeUninitialized<T>(
				GlobalHeap, newCapacity, INTRA_SOURCE_INFO);
			Span<T> newRange = newBuffer|Drop(LeftSpace())|Take(Length() + valuesLength);
			MoveInitDelete(newRange.Take(pos), mRange.Take(pos));
			MoveInitDelete(newRange|Drop(pos + valuesLength), mRange|Drop(pos));
			CopyInit(newRange|Drop(pos)|Take(valuesLength), values);
			FreeRangeUninitialized(GlobalHeap, mBuffer);
			mBuffer = newBuffer;
			mRange = newRange;
			return;
		}

		if(pos >= (Length() + valuesLength)/2 || LeftSpace() < valuesLength) //move valuesLength positions forward
		{
			mRange.End += valuesLength;
			MoveInitDeleteBackwards<T>(mRange|Drop(pos + valuesLength), mRange|Drop(pos)|DropLast(valuesLength));
		}
		else //move valuesCount positions backwards
		{
			mRange.Begin -= valuesLength;
			MoveInitDelete<T>(mRange.Take(pos), mRange|Drop(valuesLength)|Take(pos));
		}
		CopyInit<T>(mRange|Drop(pos)|Take(valuesLength), values);
	}

	template<typename U> void Insert(const T* it, Span<const U> values)
	{
		INTRA_PRECONDITION(mRange.ContainsAddress(it));
		Insert(it - mRange.Begin, values);
	}

	void Insert(Index pos, const T& value)
	{
		if(mRange.ContainsAddress(&value))
		{
			T temp = value;
			Insert(pos, Span<const T>(Unsafe, &temp, 1));
			return;
		}
		Insert(pos, {&value, 1});
	}

	void Insert(const T* it, const T& value)
	{
		INTRA_PRECONDITION(mRange.ContainsAddress(it));
		Insert(it - mRange.Begin, value);
	}


	/// Add new element to the end using stream syntax.
	ArrayList& operator<<(const T& value) {AddLast(value); return *this;}
	ArrayList& operator<<(T&& value) {AddLast(Move(value)); return *this;}

	/// Get and remove the last array element moving it to the right operand.
	ArrayList& operator>>(T& value)
	{
		value = INTRA_MOVE(Last());
		RemoveLast();
		return *this;
	}

	/// Get and remove the last array element.
	T PopLastElement()
	{
		T result = INTRA_MOVE(Last());
		RemoveLast();
		return result;
	}


	/// Pop the first element from the end.
	T PopFirstElement()
	{
		T result = INTRA_MOVE(First());
		RemoveFirst();
		return result;
	}

	/** Set new capacity of the array.
	  If rightPartSize < Length() Calls destructor for all elements with index >= rightPartSize.
	*/
	void Resize(Size rightPartSize, Size leftPartSize = 0)
	{
		if((size_t(rightPartSize)|size_t(leftPartSize)) == 0) {*this = nullptr; return;}

		// Delete all elements out of the new bounds
		if(size_t(rightPartSize) <= size_t(Length())) Destruct(mRange|Drop(rightPartSize));

		size_t newCapacity = size_t(rightPartSize) + size_t(leftPartSize);
		Span<T> newBuffer = AllocateRangeUninitialized<T>(GlobalHeap, newCapacity);
		Span<T> newRange = newBuffer|Drop(leftPartSize)|Take(Length());

		if(!mBuffer.Empty())
		{
			//Move elements to the new mBuffer
			MoveInitDelete<T>(newRange, mRange);
			FreeRangeUninitialized(GlobalHeap, mBuffer);
		}
		mBuffer = newBuffer;
		mRange = newRange;
	}

	/// Makes sure that the array has enough capacity.
	/*/
	  If it already has enough space to add at least `rightPart` - `Length()` new elements to the end
	  to add at least `leftSpace` elements to the beginning without reallocation then it does nothing.
	  Otherwise reallocates space and moves elements to a new memory allocation using move costructor and destructor.
	*/
	void Reserve(Index rightPart, Index leftSpace = 0)
	{
		const auto currentRightPartSize = size_t(mBuffer.End - mRange.Begin);
		const auto currentLeftSpace = size_t(LeftSpace());
		if(size_t(rightPart) <= currentRightPartSize && size_t(leftSpace) <= currentLeftSpace) return;

		const auto currentSize = size_t(Capacity());
		if(size_t(rightPart) > 0)
		{
			if(size_t(leftSpace) > 0) Resize(currentSize / 4 + size_t(rightPart), currentSize / 4 + size_t(leftSpace));
			else Resize(currentSize / 2 + size_t(rightPart), currentLeftSpace);
		}
		else Resize(currentRightPartSize, currentLeftSpace + currentSize/2 + size_t(leftSpace));
	}

	/// May be more comfortable alternative to Reserve.
	/// @see Reserve
	void CheckSpace(Index rightSpace, Index leftSpace = 0) {Reserve(size_t(Length()) + size_t(rightSpace), leftSpace);}

	/// Remove all array elements without freeing allocated memory.
	void Clear()
	{
		if(Empty()) return;
		Destruct(mRange);
		mRange.End = mRange.Begin;
	}

	/// @returns true if ArrayList is empty.
	constexpr bool Empty() const noexcept {return mRange.Empty();}

	/// @returns number of elements that can be inserted into the beginning of the array before reallocation is necessary.
	constexpr index_t LeftSpace() const noexcept {return mRange.Begin - mBuffer.Begin;}

	/// @returns number of elements that can be inserted into the end of the array before reallocation is necessary.
	constexpr index_t RightSpace() const noexcept {return mBuffer.End - mRange.End;}

	/*! @name Element order preserving remove operations
	  @warning: These operations invalidate all ranges, iterators and pointers referring to the elements of this ArrayList.
	*/
	///@{
	/// Remove one element at `index`.
	void Remove(Index index)
	{
		INTRA_PRECONDITION(index < Length());
		mRange[index].~T();

		// The ratio 1/4 instead of 1/2 was selected, because moving of overlapping memory
		// forward is ~2 times slower then backwards
		if(index >= size_t(Length()) / 4) //Move right part to the left
		{
			MoveInitDelete<T>(mRange|Drop(index)|DropLast(1), mRange|Drop(index + 1));
			--mRange.End;
		}
		else //Moving the left part forward
		{
			MoveInitDeleteBackwards<T>(mRange|Drop(1)|Take(index), mRange.Take(index));
			++mRange.Begin;
		}
	}

	/// Remove one element at `ptr`.
	void Remove(TUnsafe, T* ptr)
	{
		INTRA_PRECONDITION(mRange.ContainsAddress(ptr));
		Remove(ptr - mRange.Begin);
	}

	/// Remove the all elements in index mRange [`removeStart`; `removeEnd`).
	void Remove(Index removeStart, Index removeEnd)
	{
		INTRA_PRECONDITION(size_t(removeStart) <= size_t(removeEnd));
		INTRA_PRECONDITION(size_t(removeEnd) <= size_t(Length()));
		const size_t elementsToRemove = size_t(removeEnd) - size_t(removeStart);
		if(elementsToRemove == 0) return;
		Destruct<T>(mRange|Drop(removeStart)|Take(elementsToRemove));

		//Fast particular cases
		if(removeEnd == Length())
		{
			mRange.End -= elementsToRemove;
			return;
		}

		if(removeStart == 0)
		{
			mRange.Begin += elementsToRemove;
			return;
		}

		if(size_t(removeStart) + elementsToRemove / 2 >= size_t(Length()) / 4)
		{
			MoveInitDelete<T>(
				mRange|Drop(removeStart)|DropLast(elementsToRemove),
				mRange|Drop(removeEnd));
			mRange.End -= elementsToRemove;
		}
		else
		{
			MoveInitDeleteBackwards<T>(
				mRange|Drop(elementsToRemove)|Take(removeEnd - elementsToRemove),
				mRange.Take(removeStart));
			mRange.Begin += elementsToRemove;
		}
	}

	/// Find the first element equal to ``value`` and remove it.
	void FindAndRemove(const T& value)
	{
		const auto found = mRange|TakeUntil(EqualsTo(value))|Count;
		if(found != Length()) Remove(found);
	}
	///@}

	/// Remove duplicated elements from the ArrayList without keeping order of its elements.
	void RemoveDuplicatesUnordered()
	{
		for(size_t i = 0; i < size_t(Length()); i++)
			for(size_t j = i + 1; j < size_t(Length()); j++)
				if(operator[](i) == operator[](j))
					RemoveUnordered(j--);
	}

	/// Remove first ArrayList element. Complexity ``O(1)``
	void RemoveFirst() {INTRA_PRECONDITION(!Empty()); (mRange.Begin++)->~T();}

	/// Remove last ArrayList element. Complexity ``O(1)``
	void RemoveLast() {INTRA_PRECONDITION(!Empty()); (--mRange.End)->~T();}

	/// Fast O(1) remove by moving last element onto element being removed (no shift).
	void RemoveUnordered(Index index)
	{
		INTRA_PRECONDITION(size_t(index) < size_t(Length()));
		if(size_t(index) + 1 < size_t(Length())) mRange[index] = Move(*--mRange.End);
		else RemoveLast();
	}

	/// Find the first element equal to `value` and remove it by replacing it with the last element.
	void FindAndRemoveUnordered(const T& value)
	{
		const auto index = mRange|TakeUntil(EqualsTo(value))|Count;
		if(index != Length()) RemoveUnordered(index);
	}

	/// If the ratio of Capacity() / `Length()` > 125% do a reallocation to free all unused memory.
	void TrimExcessCapacity()
	{
		const auto threshold = size_t(Length()) + size_t(Length()) / 4;
		if(size_t(Capacity()) > threshold) Resize(size_t(Length()));
	}


	[[nodiscard]] T& operator[](Index index)
	{
		INTRA_PRECONDITION(size_t(index) < size_t(Length()));
		return mRange[size_t(index)];
	}

	[[nodiscard]] const T& operator[](Index index) const
	{
		INTRA_PRECONDITION(size_t(index) < size_t(Length()));
		return mRange[size_t(index)];
	}

	[[nodiscard]] T& Last()
	{
		INTRA_PRECONDITION(!Empty());
		return mRange.Last();
	}

	[[nodiscard]] const T& Last() const
	{
		INTRA_PRECONDITION(!Empty());
		return mRange.Last();
	}

	[[nodiscard]] T& First()
	{
		INTRA_PRECONDITION(!Empty());
		return mRange.First();
	}

	[[nodiscard]] const T& First() const
	{
		INTRA_PRECONDITION(!Empty());
		return mRange.First();
	}

	[[nodiscard]] T* Data() {return mRange.Data();}
	[[nodiscard]] const T* Data() const {return mRange.Data();}
	[[nodiscard]] index_t Length() const noexcept {return mRange.Length();}


	/// @return total size of ArrayList contents in bytes.
	[[nodiscard]] size_t SizeInBytes() const noexcept {return size_t(Length())*sizeof(T);}


	/** Set number of stored elements.
	  If `newCount` > `Length()` removes `newCount` - `Length()` last elements.
	  Otherwise construct `Length()` - `newCount` elements at the end using default constructor.
	*/
	void SetLength(Size newCount)
	{
		const auto oldCount = setLengthNotConstruct(newCount);
		Initialize(mRange|Drop(oldCount));
	}

	/** Set number of stored elements.
	  If `newCount` > `Length()` removes `newCount` - `Length()` last elements.
	  Otherwise construct `Length()` - `newCount` elements at the end passing `args` to its constructor.
	*/
	template<typename... Args> void SetLengthEmplace(index_t newCount, Args&... args)
	{
		const index_t oldCount = setLengthNotConstruct(newCount);
		for(T& obj: mRange|Drop(oldCount)) new(Construct, &obj) T(args...);
	}

	/** Set number of stored elements.
	  If `newCount` > `Length()` removes `newCount` - `Length()` last elements.
	  Otherwise adds `Length()` - `newCount` copies of `initValue` at the end.
	*/
	void SetLength(index_t newCount, const T& initValue)
	{
		const index_t oldCount = setLengthNotConstruct(newCount);
		for(T& dst: *this|Drop(oldCount)) new(Construct, dst) T(initValue);
	}

	/** Set number of stored elements without calling destructors and constructors.

	  Warning: Do not use it with non-POD types.
	  Otherwise calling constructors and destructors is on the caller's responsibility.
	*/
	void SetLengthUninitialized(Size newCount)
	{
		Reserve(newCount, 0);
		if(size_t(newCount) == 0) mRange.Begin = mBuffer.Begin;
		mRange.End = mRange.Begin + size_t(newCount);
	}

	/** Add `newElements` elements to the beginning of the ArrayList without initialization.
	
	  Warning: Do not use it with non-POD types.
	  Otherwise calling constructors and destructors is on the caller's responsibility.
	*/
	void AddLeftUninitialized(TUnsafe, Size newElements)
	{
		Reserve(0, newElements);
		mRange.Begin -= size_t(newElements);
	}

	/// Get current size of the mBuffer measured in elements it can store.
	[[nodiscard]] index_t Capacity() const {return mBuffer.Length();}

	[[nodiscard]] bool IsFull() const {return Length() == Capacity();}


private:
	size_t setLengthNotConstruct(Index newLength)
	{
		const auto oldLength = size_t(Length());
		if(size_t(newCount) <= oldLength)
		{
			Destruct(mRange|Drop(newLength));
			mRange.End = mRange.Begin + size_t(newLength);
			return oldCount;
		}
		SetLengthUninitialized(Unsafe, newLength);
		return oldLength;
	}

	template<typename R> void addFirstRangeHelper(R&& values)
	{
		ArrayList temp = ForwardAsRange<R>(values);
		CheckSpace(0, temp.Length());
		mRange.Begin -= temp.Length();
		MoveInit(Span<T>(Unsafe, mRange.Begin, temp.Length()), temp.AsRange());
	}

	template<typename R> void addLastRangeHelper(R&& values)
	{
		ArrayList temp = ForwardAsRange<R>(values);
		CheckSpace(temp.Length(), 0);
		MoveInit(Span(Unsafe, mRange.End, temp.Length()), Span(temp));
		mRange.End += temp.Length();
	}

	Span<T> mBuffer, mRange;
};

template<typename T> constexpr bool IsTriviallyRelocatable<ArrayList<T>> = true;

#if INTRA_CONSTEXPR_TEST
static_assert(CHasDataOf<ArrayList<int>>);
static_assert(CHasLengthOf<ArrayList<int>>);
static_assert(CHasDataOf<ArrayList<StringView>>);
static_assert(CHasDataOf<const ArrayList<StringView>&>);
static_assert(CArrayList<const ArrayList<StringView>&>);
static_assert(CTriviallyRelocatable<ArrayList<int>>);
#endif

} INTRA_END