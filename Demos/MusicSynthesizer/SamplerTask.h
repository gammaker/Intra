﻿#pragma once

#include "Intra/Type.h"
#include "Intra/Assert.h"
#include "Extra/Utils/Unique.h"
#include "Extra/Utils/FixedArray.h"
#include "Intra/Range/Mutation/Fill.h"
#include "Intra/Range/Mutation/Transform.h"
#include "Extra/Container/Utility/Blob.h"
#include "Intra/Concurrency/Atomic.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class SamplerTaskContext;

//! Задача по генерации семплов, которая прибавляет суммирует сгенерированные значения в буфер указанного контекста.
class SamplerTask
{
public:
	enum Flag: byte
	{
		LeftChannel = 1, RightChannel = 2, ReverbChannel = 4,
		ChannelMask = LeftChannel|RightChannel|ReverbChannel
	};

	uint16 OffsetInSamples, NumSamples;

	SamplerTask(size_t offsetInSamples, size_t numSamples):
		OffsetInSamples(uint16(offsetInSamples)), NumSamples(uint16(numSamples)) {}
	
	virtual ~SamplerTask() {}
	virtual void MoveConstruct(void* dst) = 0;
	virtual void operator()(SamplerTaskContext& stc) = 0;
};

typedef Intra::Container::DynamicBlob<SamplerTask, alignof(SamplerTask), uint16> SamplerTaskContainer;

//! Предварительно заполненная потокобезопасная очередь только для чтения
class SamplerTaskConsumeQueue
{
	const SamplerTaskContainer& tasks;
	Intra::Concurrency::AtomicInt index;
public:
	SamplerTaskConsumeQueue(const SamplerTaskContainer& taskContainer):
		tasks(taskContainer) {}
	SamplerTaskConsumeQueue(const SamplerTaskConsumeQueue&) = delete;
	SamplerTaskConsumeQueue(SamplerTaskConsumeQueue&&) = delete;
	SamplerTaskConsumeQueue& operator=(const SamplerTaskConsumeQueue&) = delete;
	SamplerTaskConsumeQueue& operator=(SamplerTaskConsumeQueue&&) = delete;

	SamplerTask* Dequeue()
	{
		const size_t i = size_t(index.GetIncrement());
		if(i >= tasks.Length()) return null;
		return tasks[i];
	}
};

class SamplerTaskContext
{
	FixedArray<float> allSamples;
public:
	uint16 UsedChannels;
	const Span<float> Channels[3];

	SamplerTaskContext& operator=(const SamplerTaskContext&) = delete;
	SamplerTaskContext& operator=(SamplerTaskContext&&) = delete;

	SamplerTaskContext(size_t frameLength):
		allSamples(frameLength*3), UsedChannels(0),
		Channels{
			allSamples.AsRange().Take(frameLength),
			allSamples.AsRange().Drop(frameLength).Take(frameLength),
			allSamples.AsRange().Drop(frameLength * 2)
		}
	{}

	void RunTasks(SamplerTaskConsumeQueue* queue)
	{
		for(int i = 0; i < 3; i++)
			if(UsedChannels & (1 << i))
				FillZeros(Channels[i]);
		UsedChannels = 0;
		while(SamplerTask* task = queue->Dequeue())
			(*task)(*this);
	}

	void MergeTo(SamplerTaskContext& dst) const
	{
		for(int i = 0; i < 3; i++)
		{
			if(UsedChannels & (1 << i) == 0) continue;
			INTRA_DEBUG_ASSERT(Channels[i].Length() == dst.Channels[i].Length());
			if(dst.UsedChannels & (1 << i) == 0) CopyTo(Channels[i], dst.Channels[i]);
			else Add(dst.Channels[i], Channels[i]);
		}
		dst.UsedChannels |= UsedChannels;
	}
};

INTRA_WARNING_POP
