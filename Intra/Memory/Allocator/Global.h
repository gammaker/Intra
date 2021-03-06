#pragma once

#include "Decorators/Sized.h"
#include "Decorators/BoundsChecked.h"
#include "Decorators/CallOnFail.h"
#include "System.h"
#include "Utils/Debug.h"

namespace Intra { namespace Memory {

#ifdef INTRA_DEBUG_ALLOCATORS
using SizedHeapType = ASized<ABoundsChecked<ACallOnFail<SystemHeapAllocator, NoMemoryAbort>>>;
using GlobalHeapType = ABoundsChecked<ACallOnFail<SystemHeapAllocator, NoMemoryAbort>>;
#else
using SizedHeapType = ASized<ACallOnFail<SystemHeapAllocator, NoMemoryAbort>>;
using GlobalHeapType = ACallOnFail<SystemHeapAllocator, NoMemoryAbort>;
#endif

extern SizedHeapType SizedHeap;
extern GlobalHeapType GlobalHeap;

}}
