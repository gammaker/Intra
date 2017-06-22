﻿#pragma once

#include "Cpp/Fundamental.h"
#include "Cpp/Features.h"
#include "Cpp/Warnings.h"

#include "Utils/Debug.h"

#ifndef INTRA_UTILS_NO_CONCURRENCY
#include "Concurrency/Atomic.h"
#endif

namespace Intra { namespace Utils {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

//! Более лёгкий аналог std::shared_ptr.
//! Он не принимает сырые указатели, поэтому не подвержен ошибке наличия двух счётчиков для одного объекта.
//! Выделяет и освобождает память сам через new и не хранит deleter.
//! Не поддерживает слабые ссылки.
//! Также он не может использоваться с incomplete типами.
template<typename T> class Shared
{
	struct Data
	{
		template<typename... Args> forceinline Data(Args&&... args):
			Value(Cpp::Forward<Args>(args)...), RefCount(1) {}

		forceinline void Release()
		{
			INTRA_DEBUG_ASSERT(GetRC() != 0);
			if(DecRef()) delete this;
		}
		
		T Value;
#ifndef INTRA_UTILS_NO_CONCURRENCY
		AtomicInt RefCount;
		forceinline void IncRef() {RefCount.IncrementRelaxed();}
		forceinline uint GetRC() {return uint(RefCount.GetRelaxed());}
		forceinline bool DecRef() {return RefCount.DecrementAcquireRelease() == 0;}
#else
		forceinline void IncRef() {++RefCount;}
		forceinline uint GetRC() {return RefCount;}
		forceinline bool DecRef() {return --RefCount == 0;}
		uint RefCount;
#endif

		Data(const Data&) = delete;
		Data& operator=(const Data&) = delete;
	};
	forceinline Shared(Data* data): mData(data) {}
public:
	forceinline Shared(null_t=null): mData(null) {}
	forceinline Shared(const Shared& rhs): mData(rhs.mData)
	{
		if(mData != null) mData->IncRef();
	}

	template<typename U, typename = Meta::EnableIf<
		Meta::IsInherited<U, T>::_ && Meta::HasVirtualDestructor<T>::_
	>> forceinline Shared(const Shared<U>& rhs): mData(rhs.mData)
	{
		if(mData != null) mData->IncRef();
	}

	forceinline Shared(Shared&& rhs): mData(rhs.mData) {rhs.mData = null;}
	
	template<typename U, typename = Meta::EnableIf<
		Meta::IsInherited<U, T>::_ && Meta::HasVirtualDestructor<T>::_
	>> forceinline Shared(Shared<U>&& rhs): mData(rhs.mData) {rhs.mData = null;}

	forceinline ~Shared() {if(mData != null) mData->Release();}

	Shared& operator=(const Shared& rhs)
	{
		if(mData == rhs.mData) return *this;
		if(mData != null) mData->Release();
		mData = rhs.mData;
		if(mData != null) mData->IncRef();
		return *this;
	}

	Shared& operator=(Shared&& rhs)
	{
		if(mData == rhs.mData) return *this;
		if(mData != null) mData->Release();
		mData = rhs.mData;
		rhs.mData = null;
		return *this;
	}

	forceinline Shared& operator=(null_t)
	{
		if(mData==null) return *this;
		mData->Release();
		mData = null;
		return *this;
	}

	template<typename... Args> forceinline static Shared New(Args&&... args)
	{return new Data(Cpp::Forward<Args>(args)...);}

	forceinline uint use_count() const
	{
		if(mData == null) return 0;
		return mData->GetRC();
	}

	forceinline T& operator*() const {INTRA_DEBUG_ASSERT(mData != null); return mData->Value;}
	forceinline T* operator->() const {INTRA_DEBUG_ASSERT(mData != null); return &mData->Value;}

	forceinline bool operator==(null_t) const {return mData == null;}
	forceinline bool operator!=(null_t) const {return !operator==(null);}
	forceinline bool operator==(const Shared& rhs) const {return mData == rhs.mData;}
	forceinline bool operator!=(const Shared& rhs) const {return !operator==(rhs);}

	forceinline explicit operator bool() const {return mData != null;}

private:
	Data* mData;
};

template<typename T> forceinline Shared<Meta::RemoveReference<T>> SharedMove(T&& rhs)
{return Shared<Meta::RemoveReference<T>>::New(Cpp::Move(rhs));}

INTRA_WARNING_POP

}
using Utils::Shared;
using Utils::SharedMove;

}
