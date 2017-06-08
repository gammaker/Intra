﻿#pragma once

#ifndef INTRA_NO_AUDIO_SYNTH

#include "Cpp/Warnings.h"
#include "Utils/Span.h"
#include "Container/Sequential/Array.h"
#include "Audio/Synth/Types.h"

namespace Intra { namespace Audio { namespace Synth {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class SynthesizedInstrument: public IMusicalInstrument
{
public:
	SynthesizedInstrument():
		Synth(), Modifiers(), Attenuation(), PostEffects(),
		MinNoteDuration(0), FadeOffTime(0) {}

	SynthesizedInstrument(SynthPass synth,
		CSpan<ModifierPass> modifiers=null,
		AttenuationPass attenuator=null,
		CSpan<PostEffectPass> postEffects=null,
		float minNoteDuration=0, float fadeOffTime=0);

	SynthesizedInstrument(SynthesizedInstrument&& rhs):
		Synth(Cpp::Move(rhs.Synth)),
		Modifiers(Cpp::Move(rhs.Modifiers)),
		Attenuation(Cpp::Move(rhs.Attenuation)),
		PostEffects(Cpp::Move(rhs.PostEffects)),
		MinNoteDuration(rhs.MinNoteDuration),
		FadeOffTime(rhs.FadeOffTime) {}

	SynthesizedInstrument(const SynthesizedInstrument& rhs):
		Synth(rhs.Synth),
		Modifiers(rhs.Modifiers),
		Attenuation(rhs.Attenuation),
		PostEffects(rhs.PostEffects),
		MinNoteDuration(rhs.MinNoteDuration),
		FadeOffTime(rhs.FadeOffTime) {}

	SynthesizedInstrument& operator=(const SynthesizedInstrument& rhs);
	SynthesizedInstrument& operator=(SynthesizedInstrument&& rhs);

	void GetNoteSamples(Span<float> dst, MusicNote note,
		float tempo, float volume=1, uint sampleRate=44100, bool add=false) const override;

	uint GetNoteSampleCount(MusicNote note, float tempo, uint sampleRate=44100) const override
	{
		auto noteDuration = note.AbsDuration(tempo);
		if(noteDuration<0.00001 || note.IsPause()) return 0;
		noteDuration = Math::Max(noteDuration, MinNoteDuration)+FadeOffTime;
		return uint(noteDuration*float(sampleRate));
	}

	SynthPass Synth;
	Array<ModifierPass> Modifiers;
	AttenuationPass Attenuation;
	Array<PostEffectPass> PostEffects;
	float MinNoteDuration, FadeOffTime;
};

class CombinedSynthesizedInstrument: public IMusicalInstrument
{
public:
	Array<SynthesizedInstrument> Combination;
	AttenuationPass Attenuation; //Not implemented yet
	Array<PostEffectPass> PostEffects; //Not implemented yet

	CombinedSynthesizedInstrument(CSpan<SynthesizedInstrument> instruments=null):
		Combination(instruments), Attenuation(), PostEffects() {}

	void GetNoteSamples(Span<float> dst, MusicNote note, float tempo,
		float volume=1, uint sampleRate=44100, bool add=false) const override;

	uint GetNoteSampleCount(MusicNote note, float tempo, uint sampleRate=44100) const override
	{
		uint noteSampleCount=0;
		for(auto& instr: Combination)
			noteSampleCount = Math::Max(noteSampleCount, instr.GetNoteSampleCount(note, tempo, sampleRate));
		return noteSampleCount;
	}
};

INTRA_WARNING_POP

#endif

}}}