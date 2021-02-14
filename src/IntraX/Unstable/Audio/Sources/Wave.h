﻿#pragma once

#include "Intra/Container/Optional.h"

#include "IntraX/Utils/FixedArray.h"
#include "Intra/Range/Polymorphic/OutputRange.h"

#include "IntraX/Unstable/Data/ValueType.h"
#include "IntraX/Unstable/Audio/SoundTypes.h"
#include "IntraX/Unstable/Audio/AudioSource.h"

namespace Intra { INTRA_BEGIN
namespace Sources {
class Wave: public BasicAudioSource
{
	Span<const byte> mData;
	index_t mSampleCount = 0, mCurrentDataPos = 0;
	ValueType mDataType;
public:
	Wave(OnCloseResourceCallback onClose, const SoundInfo& info, const void* data):
		BasicAudioSource(Move(onClose), int(info.SampleRate), short(info.Channels)),
		mData(SpanOfRaw(data, info.GetBufferSize())), mDataType(info.SampleType) {}

	Wave(OnCloseResourceCallback onClose, int sampleRate, short numChannels, Span<const short> data):
		BasicAudioSource(Move(onClose), sampleRate, numChannels),
		mData(data.ReinterpretUnsafe<byte>()), mSampleCount(data.Length()), mDataType(ValueType::SNorm16) {}

	Wave(OnCloseResourceCallback onClose, NonNegative<int> sampleRate, NonNegative<short> numChannels, Span<const float> data):
		BasicAudioSource(Move(onClose), sampleRate, numChannels),
		mData(data.ReinterpretUnsafe<byte>()), mSampleCount(data.Length()), mDataType(ValueType::Float) {}

	Wave(OnCloseResourceCallback onClose, Span<const byte> srcFileData);

	Wave(const Wave&) = delete;
	Wave(Wave&&) = default;
	Wave& operator=(const Wave&) = delete;
	Wave& operator=(Wave&&) = default;

	Optional<index_t> SampleCount() const override {return mSampleCount;}
	index_t SamplePosition() const override {return mCurrentDataPos/mChannelCount;}

	index_t GetInterleavedSamples(Span<short> outShorts) override;
	index_t GetInterleavedSamples(Span<float> outFloats) override;
	index_t GetUninterleavedSamples(Span<const Span<float>> outFloats) override;

	FixedArray<const void*> GetRawSamplesData(Index maxSamplesToRead,
		Optional<ValueType&> oType, Optional<bool&> oInterleaved, Optional<index_t&> oSamplesRead) override;
};

void WriteWave(IAudioSource& source, OutputStream& stream, ValueType sampleType = ValueType::SNorm16);
}
} INTRA_END