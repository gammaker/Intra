﻿#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Features.h"

#include "Utils/Span.h"

#include "Container/Sequential/Array.h"

#include "Math/Math.h"
#include "Math/SineRange.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth {

struct Chorus
{
	Array<float> DelayCircularBuffer;
	size_t CircularBufferOffset;
	float MainVolume, SecondaryVolume;
	Math::SineRange<float> Oscillator;

	Chorus(size_t maxDelaySamples, float delayFreqPerSample, float mainVolume=0.5f, float secondaryVolume=0.5f):
		DelayCircularBuffer(maxDelaySamples), CircularBufferOffset(0),
		Oscillator(maxDelaySamples*0.5f, float(-Math::PI/2), float(2*Math::PI*delayFreqPerSample)),
		MainVolume(mainVolume), SecondaryVolume(secondaryVolume) {}

	void operator()(Span<float> inOutSamples);
};

struct ChorusFactory
{
	float MaxDelay;
	float DelayFrequency;
	float MainVolume, SecondaryVolume;

	ChorusFactory(null_t=null): MaxDelay(0), DelayFrequency(0), MainVolume(0), SecondaryVolume(0) {}

	ChorusFactory(float maxDelay, float delayFreq = 3,
		float mainVolume = 0.5f, float secondaryVolume = 0.5f):
		MaxDelay(maxDelay), DelayFrequency(delayFreq),
		MainVolume(mainVolume), SecondaryVolume(secondaryVolume) {}

	Chorus operator()(float freq, float volume, uint sampleRate, size_t sampleCount) const
	{
		(void)freq; (void)volume; (void)sampleCount;
		return Chorus(size_t(MaxDelay*sampleRate), DelayFrequency/sampleRate, MainVolume, SecondaryVolume);
	}

	forceinline bool operator==(null_t) const noexcept {return MaxDelay == 0 || SecondaryVolume == 0;}
	forceinline bool operator!=(null_t) const noexcept {return !operator==(null);}
};

}}}

INTRA_WARNING_POP
