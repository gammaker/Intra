﻿#pragma once

#include "Cpp/PlatformDetect.h"

#ifndef INTRA_NO_AUDIO_SYNTH

#include "Random/FastUniform.h"
#include "Container/Utility/Array2D.h"
#include "Audio/Synth/Types.h"

namespace Intra { namespace Audio { namespace Synth { namespace Generators {

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

class DrumPhysicalModel
{
	byte mCnt;
	byte mDX, mDY;
	float mFrc, mK1, mK2;
	Array2D<float> mP, mS, mF;
	Random::FastUniform<float> mFRandom;
	float mPrevRand;

public:
	enum: bool {RangeIsInfinite = true};

	void PopFirst();

	float First() const {return mP(1, mDY/2u);}

	bool Empty() const {return mP.Width() == 0;}

	DrumPhysicalModel(null_t=null);

	DrumPhysicalModel(byte count, byte dx, byte dy, float frc, float kDemp, float kRand);
	float sRand();

	Span<float> operator()(Span<float> dst, bool add);
};

INTRA_WARNING_POP

}}}}

#endif
