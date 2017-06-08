﻿#include "Audio/Synth/TableAttenuation.h"
#include "Utils/Span.h"
#include "Range/Mutation/Copy.h"
#include "Range/Decorators/Take.h"

namespace Intra { namespace Audio { namespace Synth {

struct TableAttenuatorParams
{
	byte Len;
	Norm8 Table[23];
};

static void TableAttenuationPassFunction(const TableAttenuatorParams& table,
	float noteDuration, Span<float> inOutSamples, uint sampleRate)
{
	INTRA_DEBUG_ASSERT(table.Len>=2);
	const size_t samplesPerValue = inOutSamples.Length()/size_t(table.Len-1);

	for(uint i=0; i<table.Len-1u; i++)
	{
		double v = double(table.Table[i]);
		const double dv = (double(table.Table[i+1])-double(v))/double(samplesPerValue);
		for(size_t s=0; s<samplesPerValue; s++)
		{
			inOutSamples.First() *= float(v);
			v += dv;
			inOutSamples.PopFirst();
		}
	}
	while(!inOutSamples.Empty())
	{
		inOutSamples.First() *= float(table.Table[table.Len-1]);
		inOutSamples.PopFirst();
	}

	(void)sampleRate; (void)noteDuration;
}

AttenuationPass CreateTableAttenuationPass(CSpan<Norm8> table)
{
	TableAttenuatorParams params;
	params.Len = byte(table.Length());
	CopyTo(table, params.Table);
	return AttenuationPass(TableAttenuationPassFunction, params);
}

}}}