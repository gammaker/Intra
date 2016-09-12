﻿#include "Sound/SynthesizedInstrument.h"
#include "Sound/SoundBuilder.h"
#include "Algorithms/Algorithms.h"

#define OPTIMIZE

#if defined(OPTIMIZE) && defined(INTRA_USE_PDO)

#if INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86 || INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86_64
#include "Math/Simd.h"

#else
#undef OPTIMIZE
#undef INTRA_USE_PDO
#endif

#endif

#ifndef INTRA_NO_MIDI_SYNTH

namespace Intra {

namespace SoundSamplers
{
	Array<float> ViolinPhysicalModel::P; //Позиция участка струны
	Array<float> ViolinPhysicalModel::S; //Скорость участка струны
}

namespace SoundPostEffects
{
	Chorus::Chorus(float maxDelay, float frequency, float mainVolume, float secondaryVolume)
		{MaxDelay=maxDelay; Frequency=frequency; MainVolume=mainVolume; SecondaryVolume=secondaryVolume;}

	void Chorus::operator()(ArrayRange<float> inOutSamples, uint sampleRate) const
	{
		Array<float> copy(inOutSamples);
		float omega = Frequency*2*(float)Math::PI;
		double duration = (double)inOutSamples.Length()/sampleRate;
		float t = 0.0f;
		float dt = 1.0f/float(sampleRate);
		Math::SineRange<float> sineRange(MaxDelay, 0, omega*dt);
		auto ptr=inOutSamples.Begin;
		while(ptr<inOutSamples.End)
		{
			auto st = sineRange.First();
			sineRange.PopFirst();
			if(t>=-st && t<duration-st)
			{
				size_t index = ptr-inOutSamples.Begin+size_t(st*float(sampleRate));
				if(index>=copy.Count()) index = copy.Count()-1;
				*ptr = *ptr*MainVolume + copy[index]*SecondaryVolume;
			}
			t+=dt;
			ptr++;
		}
	}

	Echo::Echo(float delay, float mainVolume, float secondaryVolume)
		{Delay=delay; MainVolume=mainVolume; SecondaryVolume=secondaryVolume;}

	void Echo::operator()(ArrayRange<float> inOutSamples, uint sampleRate) const
	{
		Array<float> copy(inOutSamples);
		double duration=(double)inOutSamples.Length()/sampleRate;
		float t=0, dt=1.0f/float(sampleRate);
		while(!inOutSamples.Empty())
		{
			float st = t+Delay;
			if(st>=0 && st<duration)
			{
				inOutSamples.First() *= MainVolume;
				inOutSamples.First() += copy[uint(st*float(sampleRate))]*SecondaryVolume;
			}
			t+=dt;
			inOutSamples.PopFirst();
		}
	}
}



SynthesizedInstrument::SynthesizedInstrument(SoundSynthFunction synth, ArrayRange<const SoundModifierFunction> modifiers,
	SoundAttenuationFunction attenuator, ArrayRange<const SoundPostEffectFunction> postEffects, float minNoteDuration, float fadeOffTime):
	SynthPass(synth), ModifierPasses(modifiers), AttenuationPass(attenuator),
	PostEffects(postEffects), MinNoteDuration(minNoteDuration), FadeOffTime(fadeOffTime) {}

SynthesizedInstrument& SynthesizedInstrument::operator=(const SynthesizedInstrument& rhs)
{
	SynthPass = rhs.SynthPass;
	ModifierPasses = rhs.ModifierPasses;
	AttenuationPass = rhs.AttenuationPass;
	PostEffects = rhs.PostEffects;
	MinNoteDuration = rhs.MinNoteDuration;
	FadeOffTime = rhs.FadeOffTime;
	return *this;
}

SynthesizedInstrument& SynthesizedInstrument::operator=(SynthesizedInstrument&& rhs)
{
	SynthPass = rhs.SynthPass;
	ModifierPasses = core::move(rhs.ModifierPasses);
	AttenuationPass = rhs.AttenuationPass;
	PostEffects = core::move(rhs.PostEffects);
	MinNoteDuration = rhs.MinNoteDuration;
	FadeOffTime = rhs.FadeOffTime;
	return *this;
}

SoundSynthFunction SynthesizedInstrument::CreateSawtoothSynthPass(float updownRatio, float scale, ushort harmonics, float freqMultiplyer)
{
	return SoundSynthFunction(functionSawtoothSynthPass, SawtoothParams{updownRatio, harmonics, scale, freqMultiplyer});
}

SoundSynthFunction SynthesizedInstrument::CreateSineSynthPass(float scale, ushort harmonics, float freqMultiplyer)
{
	return SoundSynthFunction(functionSineSynthPass, SineParams{scale, harmonics, freqMultiplyer});
}

SoundSynthFunction SynthesizedInstrument::CreateMultiSineSynthPass(ArrayRange<const SineHarmonic> harmonics)
{
	MultiSineParams params;
	params.len = (byte)harmonics.Length();
	core::memcpy(params.harmonics, harmonics.Begin, params.len*sizeof(SineHarmonic));
	return SoundSynthFunction(functionMultiSineSynthPass, params);
}

SoundSynthFunction SynthesizedInstrument::CreateSineExpSynthPass(ArrayRange<const SineExpHarmonic> harmonics)
{
	SineExpParams params;
	params.len = (byte)harmonics.Length();
	core::memcpy(params.harmonics, harmonics.Begin, params.len*sizeof(SineExpHarmonic));
	return SoundSynthFunction(functionSineExpSynthPass, params);
}



#include <malloc.h>

void SynthesizedInstrument::GetNoteSamples(ArrayRange<float> inOutSamples,
	MusicNote note, float tempo, float volume, uint sampleRate, bool add) const
{
	INTRA_ASSERT(SynthPass!=null);
	if(SynthPass==null) return;
	auto noteDuration=note.AbsDuration(tempo);
	if(noteDuration<0.00001 || note.IsPause() || volume==0) return;
	noteDuration = Math::Max(noteDuration, MinNoteDuration)+FadeOffTime;

	size_t noteSampleCount = inOutSamples.Length();//Math::Min(inOutSamples.Length(), size_t(noteDuration*sampleRate));
	if(ModifierPasses==null && AttenuationPass==null && PostEffects==null) //Если синтез однопроходный, то промежуточный буфер не нужен
	{
		SynthPass(note.Frequency(), volume, inOutSamples.Take(noteSampleCount), sampleRate, add);
	}
	else
	{
		Array<float> noteSampleArr;
		noteSampleArr.SetCountUninitialized(noteSampleCount);
		auto sampleBuf = noteSampleArr(0, noteSampleCount);
		SynthPass(note.Frequency(), volume, sampleBuf, sampleRate, false);
		for(auto& pass: ModifierPasses)
		{
			if(pass==null) continue;
			pass(note.Frequency(), sampleBuf, sampleRate);
		}
		if(AttenuationPass!=null) AttenuationPass(noteDuration, sampleBuf/*result.Samples*/, sampleRate);
		for(auto& postEffect: PostEffects)
		{
			if(postEffect==null) continue;
			postEffect(sampleBuf, sampleRate);
		}
		if(!add) Memory::CopyBits(inOutSamples.Take(noteSampleCount), sampleBuf.AsConstRange());
		else Algo::Add(inOutSamples.Take(noteSampleCount), sampleBuf.AsConstRange());
	}
}

void CombinedSynthesizedInstrument::GetNoteSamples(ArrayRange<float> inOutSamples, MusicNote note, float tempo, float volume, uint sampleRate, bool add) const
{
	if(Combination==null || note.IsPause()) return;
	int i=0;
	for(auto& instr: Combination)
	{
		instr.GetNoteSamples(inOutSamples, note, tempo, volume, sampleRate, add || i>0);
		i++;
	}
}


static void perfect_sawtooth(double upPercent, float volume, float freq, uint sampleRate, ArrayRange<float> inOutSamples, bool add)
{
	SoundSamplers::Sawtooth saw((float)(upPercent/(1-upPercent)));
	saw.SetParams(freq, volume, 1.0/sampleRate);
	if(!add) for(auto& sample: inOutSamples) sample = saw.NextSample();
	else for(auto& sample: inOutSamples) sample += saw.NextSample();
}


static uint get_good_signal_period(double samplesPerPeriod, uint maxPeriods)
{
	if(Math::Fract(samplesPerPeriod)<=0.05) return 1; //Подбираем количество периодов так, чтобы их было не очень много, но конец переходил в начало с минимальным швом
	double fractionalCount = 1.0/(Math::Floor(Math::Fract(samplesPerPeriod)*20)/20);
	double minDeltaCnt=1;
	uint minDeltaN=0;
	for(int n=1; fractionalCount*n<maxPeriods || minDeltaCnt>0.1f; n++)
	{
		double delta = Math::Fract(fractionalCount*n);
		if(delta>0.5) delta=1-delta;
		delta/=n;
		if(minDeltaCnt>delta)
		{
			minDeltaCnt=delta;
			minDeltaN=n;
		}
	}
	return (uint)Math::Round(fractionalCount*minDeltaN);
}

static void repeat_fragment_in_buffer(ArrayRange<const float> fragmentSamples, ArrayRange<float> inOutSamples, bool add)
{
	const size_t copyPassCount = inOutSamples.Length()/fragmentSamples.Length();
	float* ptr = inOutSamples.Begin;
	if(!add) for(size_t c=0; c<copyPassCount; c++)
	{
		core::memcpy(ptr, fragmentSamples.Begin, fragmentSamples.Length()*sizeof(fragmentSamples[0]));
		ptr += fragmentSamples.Length();
	}
	else for(size_t c=0; c<copyPassCount; c++)
	{
		const float* src = fragmentSamples.Begin;
		while(src<fragmentSamples.End) *ptr++ += *src++;
	}

	const float* src = fragmentSamples.Begin;
	if(!add) core::memcpy(ptr, src, (inOutSamples.End-ptr)*sizeof(fragmentSamples[0]));
	else while(ptr<inOutSamples.End) *ptr++ += *src++;
}

static void fast_sawtooth(double upPercent, float volume, float freq, uint sampleRate, ArrayRange<float> inOutSamples, bool add)
{
	const double samplesPerPeriod = float(sampleRate)/freq;
	uint count = get_good_signal_period(samplesPerPeriod, Math::Max(uint(freq/50), 5u));

	//Генерируем фрагмент, который будем повторять, пока не заполним буфер целиком
	SoundBuffer samples;
	const auto sampleCount = (uint)Math::Round(samplesPerPeriod*count);
	samples.Samples.SetCountUninitialized(sampleCount);
	perfect_sawtooth(upPercent, volume, freq, sampleRate, samples.Samples, false);

	repeat_fragment_in_buffer(samples.Samples, inOutSamples, add);
}



static void perfect_sine(float volume, float freq, uint sampleRate, ArrayRange<float> inOutSamples, bool add)
{
	Math::SineRange<float> sineRange(volume, 0, float(2*Math::PI*freq/sampleRate));
	if(!add) while(!inOutSamples.Empty())
	{
		inOutSamples.First() = sineRange.First();
		inOutSamples.PopFirst();
		sineRange.PopFirst();
	}
	else while(!inOutSamples.Empty())
	{
		inOutSamples.First() += sineRange.First();
		inOutSamples.PopFirst();
		sineRange.PopFirst();
	}
}

static void fast_sine(float volume, float freq, uint sampleRate, ArrayRange<float> inOutSamples, bool add)
{
	const double samplesPerPeriod = float(sampleRate)/freq;
	uint count = get_good_signal_period(samplesPerPeriod, Math::Max((uint)(freq/50), 5u));

	//Генерируем фрагмент, который будем повторять, пока не заполним буфер целиком
	SoundBuffer samples;
	samples.SampleRate = sampleRate;
	const auto sampleCount = (uint)Math::Round(samplesPerPeriod*count);
	samples.Samples.SetCountUninitialized(sampleCount);
	perfect_sine(volume, freq, sampleRate, samples.Samples, false);

	repeat_fragment_in_buffer(samples.Samples, inOutSamples, add);
}

#if INTRA_DISABLED
static void exponent_attenuation_inplace(float*& ptr, float* end, float& exp, float ek)
{
#ifndef OPTIMIZE

#elif !defined(INTRA_USE_PDO)
	float ek8=ek*ek;
	ek8*=ek8;
	ek8*=ek8;
	while(ptr<end-7)
	{
		*ptr++*=exp;
		*ptr++*=exp;
		*ptr++*=exp;
		*ptr++*=exp;
		*ptr++*=exp;
		*ptr++*=exp;
		*ptr++*=exp;
		*ptr++*=exp;
		exp*=ek8;
	}
#elif PLATFORM==INTRA_PLATFORM_X86 || PLATFORM==INTRA_PLATFORM_X86_64
	float ek4 = ek*ek;
	ek4 *= ek4;
	float4 ek_4 = Simd::Set(ek4);
	float4 exp_4 = Simd::Set(1, ek, ek*ek, ek*ek*ek);
	//_mm_prefetch((const char*)ptr, _MM_HINT_NTA);
	while(ptr<end-3)
	{
		Simd::GetU(ptr, Simd::Mul(Simd::SetU(ptr), exp_4));
		exp_4 = Simd::Mul(exp_4, ek_4);
		ptr+=4;
	}
	exp = Simd::GetX(exp_4);
#elif PLATFORM==INTRA_PLATFORM_ARM

#endif
	while(ptr<end) *ptr++*=exp, exp*=ek;
}
#endif

static void exponent_attenuation(float*& dst, float* src, float* dstend, float& exp, float ek)
{
#ifndef OPTIMIZE

#elif !defined(INTRA_USE_PDO)
	float ek8=ek*ek;
	ek8*=ek8;
	ek8*=ek8;
	while(dst<dstend-7)
	{
		*dst++ = *src++ * exp;
		*dst++ = *src++ * exp;
		*dst++ = *src++ * exp;
		*dst++ = *src++ * exp;
		*dst++ = *src++ * exp;
		*dst++ = *src++ * exp;
		*dst++ = *src++ * exp;
		*dst++ = *src++ * exp;
		exp*=ek8;
	}
#elif INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86 || INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86_64
	float ek4=ek*ek;
	ek4*=ek4;
	Simd::float4 ek_4 = Simd::Set(ek4);
	//while(((size_t)dst)&15 && dst<dstend) *dst++ = *src++ * Exp, Exp*=ek;
	Simd::float4 exp_4 = Simd::Set(exp, exp*ek, exp*ek*ek, exp*ek*ek*ek);
	/*while(dst<dstend-3)
	{
		Simd::GetU(dst, Simd::Mul(Simd::SetU(src), exp_4));
		exp_4 = Simd::Mul(exp_4, ek_4);
		dst+=4; src+=4;
	}
	Exp = Simd::GetX(exp_4);*/

	Simd::float4 r0, r1, ek_8, r3, r4;
	r1=exp_4;
	ek_8=ek_4;
	r4 = Simd::Mul(ek_8, r1);
	ek_8 = Simd::Mul(ek_8, ek_8);
	while(dst<dstend-7)
	{
		r0 = Simd::Mul(Simd::SetU(src), r1);
		r1 = Simd::Mul(r1, ek_8);
		r3 = Simd::Mul(Simd::SetU(src+4), r4);
		Simd::GetU(dst, r0);
		r4 = Simd::Mul(r4, ek_8);
		Simd::GetU(dst+4, r3);
		dst+=8; src+=8;
	}
	exp = Simd::GetX(r1);
#elif PLATFORM==INTRA_PLATFORM_ARM

#endif
	while(dst<dstend)
		*dst++ = *src++ * exp, exp*=ek;
}



static void exponent_attenuation_add(float*& dst, float* src, float* dstend, float& exp, float ek)
{
#ifndef OPTIMIZE

#elif !defined(INTRA_USE_PDO)
	float ek8 = ek*ek;
	ek8 *= ek8;
	ek8 *= ek8;
	while(dst<dstend-7)
	{
		*dst++ += *src++ * exp;
		*dst++ += *src++ * exp;
		*dst++ += *src++ * exp;
		*dst++ += *src++ * exp;
		*dst++ += *src++ * exp;
		*dst++ += *src++ * exp;
		*dst++ += *src++ * exp;
		*dst++ += *src++ * exp;
		exp*=ek8;
	}
#elif PLATFORM==INTRA_PLATFORM_X86 || PLATFORM==INTRA_PLATFORM_X86_64
	//while(((size_t)dst)&15 && dst<dstend) *dst++ += *src++ * Exp, Exp*=ek;
	float ek4 = ek*ek;
	ek4 *= ek4;
	Simd::float4 ek_4 = Simd::Set(ek4);
	Simd::float4 exp_4 = Simd::Set(exp, exp*ek, exp*ek*ek, exp*ek*ek*ek);
	/*while(dst<dstend-7)
	{
		Simd::GetU(dst, Simd::Add(Simd::SetU(dst), Simd::Mul(Simd::SetU(src), exp_4)));
		exp_4 = Simd::Mul(exp_4, ek_4);
		dst+=4; src+=4;
	}
	Exp = Simd::GetX(exp_4);*/

	Simd::float4 r0, r1, ek_8, r3, r4;
	r1=exp_4;
	ek_8=ek_4;
	r4 = Simd::Mul(ek_8, r1);
	ek_8 = Simd::Mul(ek_8, ek_8);
	while(dst<dstend-7)
	{
		r0 = Simd::Mul(Simd::SetU(src), r1);
		r1 = Simd::Mul(r1, ek_8);
		r3 = Simd::Mul(Simd::SetU(src+4), r4);
		Simd::GetU(dst, Simd::Add(Simd::SetU(dst), r0));
		r4 = Simd::Mul(r4, ek_8);
		Simd::GetU(dst+4, Simd::Add(Simd::SetU(dst+4), r3));
		dst+=8; src+=8;
	}
	exp = Simd::GetX(r1);
#elif PLATFORM==INTRA_PLATFORM_ARM

#endif
	while(dst<dstend) *dst++ += *src++ * exp, exp*=ek;
}

static void fast_sinexp(float volume, float coeff, float freq, uint sampleRate, ArrayRange<float> inOutSamples, bool add)
{
#if INTRA_DISABLED
	if(!add) f1_sse(inOutSamples.Begin, inOutSamples.Count(), coeff/sampleRate, (float)(2*Math::PI*freq/sampleRate), volume, 0);
	else f1_sse_add(inOutSamples.Begin, inOutSamples.Count(), coeff/sampleRate, (float)(2*Math::PI*freq/sampleRate), volume, 0);
#endif
#if INTRA_DISABLED
	float phi0=0, dphi=(float)(2*Math::PI*freq/sampleRate);
	float S0=volume*sinf(phi0), S1=volume*sinf(dphi);
	float K=2*expf(-coeff/sampleRate)*cosf(dphi), b=expf(-2*coeff/sampleRate);
	auto ptr=inOutSamples.Begin, end=inOutSamples.end;

#ifdef OPTIMIZE
#ifndef INTRA_USE_PDO
#elif PLATFORM==INTRA_PLATFORM_X86 || PLATFORM==INTRA_PLATFORM_X86_64
	float K4=2*expf(-4*coeff/sampleRate)*cosf(4*dphi), b4=expf(-8*coeff/sampleRate);

	Simd::float4 vS0 = Simd::Set(volume*Sin(phi0), volume*sinf(phi0+dphi)*expf(-2*coeff/sampleRate), volume*sinf(phi0+2*dphi)*expf(-4*coeff/sampleRate), volume*sinf(phi0+3*dphi)*expf(-6*coeff/sampleRate));
	Simd::float4 vS1 = Simd::Set(volume*sinf(4*dphi));
	Simd::float4 vK4 = Simd::Set(K4);
	Simd::float4 vb4 = Simd::Set(b4);

	if(!add) while(ptr<end-3)
	{
		Simd::GetU(ptr, vS1);
		ptr+=4;

		Simd::float4 newvS = Simd::Sub(Simd::Mul(vK4, vS1), vS0);
		vS0 = Simd::Mul(vS1, vb4);
		vS1 = newvS;
	}
	else while(ptr<end-3)
	{
		Simd::GetU(ptr, Simd::Add(Simd::SetU(ptr), vS1));
		ptr+=4;

		Simd::float4 newvS = Simd::Sub(Simd::Mul(vK4, vS1), vS0);
		vS0 = Simd::Mul(vS1, vb4);
		vS1=newvS;
	}

	int i=0;
	if(!add) while(ptr<end)
		*ptr++ = vS1.m128_f32[i++];
	else while(ptr<end)
		*ptr++ += vS1.m128_f32[i++];
#else

#endif
#endif

	if(!add) while(ptr<end)
	{
		*ptr++ = S1;

		const auto newS=K*S1-S0;
		S0=S1*b;
		S1=newS;
	}
	else while(ptr<end)
	{
		*ptr++ += S1;

		const auto newS=K*S1-S0;
		S0=S1*b;
		S1=newS;
	}
#endif
//#if INTRA_DISABLED
	const double samplesPerPeriod = float(sampleRate)/freq;
	uint count = get_good_signal_period(samplesPerPeriod, Math::Max((uint)(freq/50), 5u));

	//Генерируем фрагмент, который будем повторять, пока не заполним буфер целиком
	auto sampleCount = (uint)Math::Round(samplesPerPeriod*count), N=(500/sampleCount+1);
	SoundBuffer samples;
	samples.SampleRate = sampleRate;
	samples.Samples.SetCountUninitialized(sampleCount*N);
	perfect_sine(volume, freq, sampleRate, samples.Samples(0, sampleCount), false);

	//Если этот фрагмент короче 500 семплов, то повторим его, там как экспоненциальное затухание эффективнее для больших массивов
	for(uint i=1; i<N; i++) samples.CopyFrom(i*sampleCount, sampleCount, &samples, 0);

	float ek = Math::Exp(-coeff/float(sampleRate));
	float exp = 1.0f;
	auto ptr=inOutSamples.Begin;
	if(!add) while(ptr<inOutSamples.End-samples.Samples.Count()+1)
		exponent_attenuation(ptr, samples.Samples.begin(), ptr+samples.Samples.Count(), exp, ek);
	else while(ptr<inOutSamples.End-samples.Samples.Count()+1)
		exponent_attenuation_add(ptr, samples.Samples.begin(), ptr+samples.Samples.Count(), exp, ek);

	if(!add) exponent_attenuation(ptr, samples.Samples.begin(), inOutSamples.End, exp, ek);
	else exponent_attenuation_add(ptr, samples.Samples.begin(), inOutSamples.End, exp, ek);
	INTRA_ASSERT(ptr==inOutSamples.End);
//#endif
}


#define sawtooth fast_sawtooth
#define sine_generate fast_sine
#define sinexp_generate fast_sinexp
//#define sawtooth perfect_sawtooth
//#define sine_generate perfect_sine

void SynthesizedInstrument::functionSawtoothSynthPass(const SawtoothParams& params,
		float freq, float volume, ArrayRange<float> inOutSamples, uint sampleRate, bool add)
{
	if(inOutSamples==null) return;
	double updownPercent = params.updownRatio/(params.updownRatio+1);
	float newFreq=freq*params.freqMultiplyer;
	float maxValue=1, harmVal=1;
	for(ushort h=1; h<params.harmonics; h++) maxValue+=(harmVal/=2);
	float newVolume=volume*params.scale/maxValue;

	sawtooth(updownPercent, newVolume, newFreq, sampleRate, inOutSamples, add);

	Math::Random<float> frandom(612651278);
	for(ushort h=1; h<params.harmonics; h++)
	{
		newVolume/=2;
		float frequency = newFreq*float(1 << h);
		frequency += frandom()*frequency*0.002f;
		size_t randomSampleOffset = Math::Min<size_t>(Math::Random<ushort>::Global(20), inOutSamples.Length());
		sawtooth(updownPercent, newVolume, frequency, sampleRate, inOutSamples.Drop(randomSampleOffset), true);
	}
}

void SynthesizedInstrument::functionSineSynthPass(const SineParams& params,
	float freq, float volume, ArrayRange<float> inOutSamples, uint sampleRate, bool add)
{
	if(inOutSamples==null) return;
	float newFreq=freq*params.freqMultiplyer;
	float maxValue=1, harmVal=1;
	for(ushort h=1; h<params.harmonics; h++) maxValue+=(harmVal/=2);
	float newVolume=volume*params.scale/maxValue;

	sine_generate(newVolume, newFreq, sampleRate, inOutSamples, add);

	Math::Random<float> frandom(1278328923);
	for(ushort h=1; h<params.harmonics; h++)
	{
		newVolume/=2;
		float frequency = newFreq*float(1 << h);
		frequency += frandom()*frequency*0.002f;
		sine_generate(newVolume, frequency, sampleRate, inOutSamples.Drop(Math::Random<ushort>::Global(20)), true);
	}
}

void SynthesizedInstrument::functionMultiSineSynthPass(const MultiSineParams& params,
	float freq, float volume, ArrayRange<float> inOutSamples, uint sampleRate, bool add)
{
	if(inOutSamples==null) return;
	size_t start = Math::Random<ushort>::Global(20);
	if(start>inOutSamples.Length()) start=inOutSamples.Length();
	if(!add) core::memset(inOutSamples.Begin, 0, start*sizeof(float));
	for(ushort h=0; h<params.len; h++)
	{
		auto& harm = params.harmonics[h];
		sine_generate(volume*(float)harm.scale,
			freq*(float)harm.freqMultiplyer, sampleRate,
			inOutSamples.Drop(start),
			add || h>0);
	}
}

void SynthesizedInstrument::functionSineExpSynthPass(const SineExpParams& params,
	float freq, float volume, ArrayRange<float> inOutSamples, uint sampleRate, bool add)
{
	if(inOutSamples==null) return;
	size_t start = Math::Random<ushort>::Global(20);
	if(start>inOutSamples.Length()) start=inOutSamples.Length();
	if(!add)
	{
		core::memset(inOutSamples.Begin, 0, start*sizeof(float));
		if(params.harmonics[0].lengthMultiplyer<norm8s(1))
		{
			size_t samplesToProcess = size_t(float(inOutSamples.Length()-start)*(float)params.harmonics[0].lengthMultiplyer);
			size_t freeSamplesLeft = inOutSamples.Length()-start-samplesToProcess;
			core::memset(inOutSamples.Begin+start+samplesToProcess, 0, freeSamplesLeft*sizeof(float));
		}
	}
	for(ushort h=0; h<params.len; h++)
	{
		auto& harm = params.harmonics[h];
		size_t samplesToProcess = size_t(float(inOutSamples.Length()-start)*(float)harm.lengthMultiplyer);
		sinexp_generate(volume*(float)harm.scale, (float)harm.attenCoeff,
			freq*(float)harm.freqMultiplyer, sampleRate,
			inOutSamples.Drop(start).Take(samplesToProcess),
			add || h>0);
	}
}

void SynthesizedInstrument::functionADPass(const ADParams& params, float noteDuration, ArrayRange<float> inOutSamples, uint sampleRate)
{
	const double halfAttackTime = Math::Min(noteDuration*0.5, params.attackTime)*0.5;
	const double halfDecayTime = Math::Min(noteDuration*0.5, params.decayTime)*0.5;
	const uint halfAttackSamples = uint(halfAttackTime*sampleRate), attackSampleEnd=halfAttackSamples*2;
	const double decayTimeBegin = noteDuration-halfDecayTime*2;
	const uint halfDecaySamples = uint(halfDecayTime*sampleRate);
	const uint decaySampleBegin = uint(decayTimeBegin*sampleRate);

	auto ptr = inOutSamples.Begin;
	auto endHalfAttack = inOutSamples.Begin+halfAttackSamples;
	auto endAttack = inOutSamples.Begin+attackSampleEnd;
	auto beginDecay = inOutSamples.Begin+decaySampleBegin;
	auto beginHalfDecay = beginDecay+halfDecaySamples;

	float u, du;

#if !defined(OPTIMIZE)
	u = 0;
	du = sqrtf(0.5f)/halfAttackSamples;
	while(ptr<endHalfAttack) *ptr++*=u*u, u+=du;

	u = 0;
	du = 0.25f/halfAttackSamples;
	while(ptr<endAttack) *ptr++*=sqrtf(u)+0.5f, u+=du;

	ptr=beginDecay;
	u = 0.25f;
	du = -0.25f/halfDecaySamples;
	while(ptr<beginHalfDecay) *ptr++*=sqrtf(u)+0.5f, u+=du;

	u = sqrtf(0.5f);
	du = -sqrtf(0.5f)/halfDecaySamples;
	while(ptr<inOutSamples.end) *ptr++*=u*u, u+=du;
#elif !defined(INTRA_USE_PDO)
	float du4;

	u = 0;
	du = 0.707107f/float(halfAttackSamples);
	du4 = 4*du;
	while(ptr<endHalfAttack-3)
	{
		float u2 = u*u;
		*ptr++ *= u2;
		*ptr++ *= u2;
		*ptr++ *= u2;
		*ptr++ *= u2;
		u+=du4;
	}
	while(ptr<endHalfAttack) *ptr++ *= u*u, u += du;

	u = 0;
	du = 0.25f/float(halfAttackSamples);
	du4 = 4.0f*du;
	while(ptr<endAttack-3)
	{
		float u2 = Math::Sqrt(u)+0.5f;
		*ptr++ *= u2;
		*ptr++ *= u2;
		*ptr++ *= u2;
		*ptr++ *= u2;
		u += du4;
	}
	while(ptr<endAttack) *ptr++ *= Math::Sqrt(u)+0.5f, u+=du;

	ptr=beginDecay;
	u = 0.25f;
	du = -0.25f/float(halfDecaySamples);
	du4 = 4.0f*du;
	while(ptr<beginHalfDecay-3)
	{
		float u2 = Math::Sqrt(u)+0.5f;
		*ptr++ *= u2;
		*ptr++ *= u2;
		*ptr++ *= u2;
		*ptr++ *= u2;
		u += du4;
	}
	while(ptr<beginHalfDecay) *ptr++ *= Math::Sqrt(u)+0.5f, u+=du;

	u = 0.707107f;
	du = -0.707107f/float(halfDecaySamples);
	du4 = 4.0f*du;
	while(ptr<inOutSamples.End-3)
	{
		float u2 = u*u;
		*ptr++ *= u2;
		*ptr++ *= u2;
		*ptr++ *= u2;
		*ptr++ *= u2;
		u += du4;
	}
	while(ptr<inOutSamples.End) *ptr++ *= u*u, u += du;
#elif INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86 || INTRA_PLATFORM_ARCH==INTRA_PLATFORM_X86_64
	//Атака
	
	//Первая половина атаки
	du = 0.707107f/halfAttackSamples;
	Simd::float4 u4 = Simd::Set(0, du, 2*du, 3*du);
	Simd::float4 du4 = Simd::Set(4*du);
	while(ptr<endHalfAttack-3)
	{
		Simd::float4 v = Simd::SetU(ptr);
		Simd::float4 res = Simd::Mul(u4, u4);
		Simd::GetU(ptr, Simd::Mul(v, res));
		u4 = Simd::Add(u4, du4);
		ptr += 4;
	}
	while(ptr<endHalfAttack)
	{
		const float u_x = Simd::GetX(u4);
		*ptr++ *= u_x*u_x;
		u4 = Simd::Set(u_x+du);
	}

	//Вторая половина атаки
	du = 0.25f/halfAttackSamples;
	u4 = Simd::Set(0, du, 2*du, 3*du);
	du4 = Simd::Set(4*du);
	Simd::float4 half = Simd::Set(0.5f);
	while(ptr<endAttack-3)
	{
		Simd::float4 v = Simd::SetU(ptr);
		Simd::float4 res = Simd::Add(Simd::Sqrt(u4), half);
		Simd::GetU(ptr, Simd::Mul(v, res));
		u4 = Simd::Add(u4, du4);
		ptr+=4;
	}
	float u4x = Simd::GetX(u4);
	while(ptr<endAttack)
	{
		*ptr++ *= Math::Sqrt(u4x)+0.5f;
		u4x += du;
	}


	//Спад
	ptr = beginDecay;

	//Первая половина спада
	u = 0.25f;
	du = -0.25f/halfDecaySamples;
	//u4 = Simd::Set(0.5f, 0.5f-du/2, 0.5f-du, 0.5f-3*du/2);
	u4 = Simd::Set(u, u+du, u+2*du, u+3*du);
	du4 = Simd::Set(4*du);
	while(ptr<beginHalfDecay-3)
	{
		Simd::float4 v = Simd::SetU(ptr);
		Simd::float4 Sqrt = Simd::Sqrt(u4);
		Simd::float4 res = Simd::Add(Sqrt, half);
		Simd::GetU(ptr, Simd::Mul(v, res));
		u4 = Simd::Add(u4, du4);
		ptr+=4;
	}
	float u4vals[4]; Simd::GetU(u4vals, u4);
	float u4w = u4vals[3];
	while(ptr<beginHalfDecay)
	{
		*ptr++ *= Math::Sqrt(u4w)+0.5f;
		u4w += du;
	}

	//Вторая половина спада
	u = 0.707107f;
	du = -0.707107f/halfDecaySamples;
	u4 = Simd::Set(u, u+du, u+2*du, u+3*du);
	du4 = Simd::Set(4*du);
	while(ptr<inOutSamples.End-3)
	{
		Simd::float4 v = Simd::SetU(ptr);
		Simd::float4 res = Simd::Mul(u4, u4);
		Simd::GetU(ptr, Simd::Mul(v, res));
		u4 = Simd::Add(u4, du4);
		ptr+=4;
	}
	u4x = Simd::GetX(u4);
	while(ptr<inOutSamples.End) *ptr++ *= u4x*u4x;
#elif INTRA_PLATFORM_ARCH==INTRA_PLATFORM_ARM

#endif
}



void SynthesizedInstrument::functionExpAttenuationPass(const float& coeff,
	float noteDuration, ArrayRange<float> inOutSamples, uint sampleRate)
{
	auto ptr = inOutSamples.Begin;
	auto end = inOutSamples.End;
	float ek = Math::Exp(-coeff/float(sampleRate));
	float exp = 1.0f;
	exponent_attenuation(ptr, ptr, end, exp, ek);
	(void)noteDuration;
}

void SynthesizedInstrument::functionTableAttenuationPass(const TableAttenuatorParams& table,
	float noteDuration, ArrayRange<float> inOutSamples, uint sampleRate)
{
	INTRA_ASSERT(table.len>=2);
	const size_t samplesPerValue = inOutSamples.Length()/(table.len-1);
	auto pos = inOutSamples.Begin;

	for(uint i=0; i<table.len-1u; i++)
	{
		double v = (double)table.table[i];
		double dv = (double(table.table[i+1])-double(v))/double(samplesPerValue);
		for(size_t s=0; s<samplesPerValue; s++) *pos++ *= (float)v, v+=dv;
	}
	while(pos<inOutSamples.End) (*pos++) *= (float)table.table[table.len-1];

	(void)sampleRate; (void)noteDuration;
}


SoundAttenuationFunction SynthesizedInstrument::CreateADPass(double attackTime, double decayTime)
{
	return SoundAttenuationFunction(functionADPass, ADParams{attackTime, decayTime});
}

SoundAttenuationFunction SynthesizedInstrument::CreateExpAttenuationPass(float coeff)
{
	return SoundAttenuationFunction(functionExpAttenuationPass, coeff);
}

SoundAttenuationFunction SynthesizedInstrument::CreateTableAttenuationPass(ArrayRange<const norm8> table)
{
	TableAttenuatorParams params;
	params.len = (byte)table.Length();
	Memory::CopyBits(ArrayRange<norm8>(params.table, params.len), table);
	return SoundAttenuationFunction(functionTableAttenuationPass, params);
}



void DrumInstrument::GetNoteSamples(ArrayRange<float> inOutSamples,
	MusicNote note, float tempo, float volume, uint sampleRate, bool add) const
{
	uint id = note.Octave*12+uint(note.Note);
	SoundBuffer buf;
	bool exists;
	auto gen = Generators.Get(id);
	auto& bufRef = SamplesCache.Get(gen, &exists);
	if(exists && bufRef.SampleRate==sampleRate &&
		note.AbsDuration(tempo)<=bufRef.Duration()) buf = bufRef;
	else
	{
		if(gen==null) return;
		size_t newSampleCount = GetNoteSampleCount(note, tempo, sampleRate);
		buf.Samples.SetCountUninitialized(exists? newSampleCount+bufRef.Samples.Count()/2: newSampleCount);
		buf.SampleRate = sampleRate;
		gen->GetNoteSamples(buf.Samples, MusicNote(4, MusicNote::NoteType::C, ushort(note.Duration*tempo)), 1, 1, sampleRate, false);
		SamplesCache[gen] = buf;
	}
	if(Math::Abs(volume-1.0f)>0.001f) Algo::Multiply(buf.Samples.AsRange(), volume);
	auto sampleCount = Math::Min(buf.Samples.Count(), inOutSamples.Length());
	if(!add) Memory::CopyBits(inOutSamples.Take(buf.Samples.Count()), buf.Samples.AsConstRange());
	else Algo::Add(inOutSamples.Take(sampleCount), buf.Samples(0, sampleCount).AsConstRange());
}

}


#endif
