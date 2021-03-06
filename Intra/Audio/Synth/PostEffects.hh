﻿#pragma once

#include "Cpp/Warnings.h"
#include "Cpp/Features.h"

#include "Utils/Span.h"

#include "Math/SineRange.h"

#include "Types.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

namespace Intra { namespace Audio { namespace Synth { namespace PostEffects {

struct Echo
{
	float Delay;
	float MainVolume, SecondaryVolume;

	Echo(float delay=0.03f, float mainVolume=0.5f, float secondaryVolume=0.5f):
		Delay(delay), MainVolume(mainVolume), SecondaryVolume(secondaryVolume) {}

	void operator()(Span<float> inOutSamples, uint sampleRate) const;
};

struct FilterDrive
{
	float K;
	FilterDrive(float k): K(k) {}

	void operator()(Span<float> inOutSamples, uint sampleRate) const;
};

struct FilterHP
{
	float K;
	FilterHP(float k): K(k) {}

	void operator()(Span<float> inOutSamples, uint sampleRate) const;
};


struct FilterQ
{
	float Frq, K;
	FilterQ(float frq, float k): Frq(frq), K(k) {}

	void operator()(Span<float> samples, uint sampleRate) const;
};

struct Fade
{
	uint FadeIn, FadeOut;

	Fade(uint fadeIn=0, uint fadeOut=0):
		FadeIn(fadeIn), FadeOut(fadeOut) {}

	void operator()(Span<float> inOutSamples, uint sampleRate) const;
};

}}}}

INTRA_WARNING_POP
