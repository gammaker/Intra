﻿#pragma once

#include "Utils/Delegate.h"
#include "Utils/Span.h"
#include "Audio/MusicNote.h"

namespace Intra { namespace Audio {

struct MusicTrack;
namespace Synth {

class IMusicalInstrument
{
public:
	virtual ~IMusicalInstrument() {}

	virtual void GetNoteSamples(Span<float> dst,
		MusicNote note, float tempo, float volume=1, uint sampleRate=44100, bool add=false) const = 0;

	virtual uint GetNoteSampleCount(MusicNote note, float tempo, uint sampleRate=44100) const = 0;
	virtual void PrepareToPlay(const MusicTrack& /*track*/, uint /*sampleRate*/) const {}
};

typedef Delegate<void(float freq, float volume, Span<float> outSamples, uint sampleRate, bool add)> SynthPass;
typedef Delegate<void(float freq, Span<float> inOutSamples, uint sampleRate)> ModifierPass;
typedef Delegate<void(float noteDuration, Span<float> inOutSamples, uint sampleRate)> AttenuationPass;
typedef Delegate<void(Span<float> inOutSamples, uint sampleRate)> PostEffectPass;

}}}