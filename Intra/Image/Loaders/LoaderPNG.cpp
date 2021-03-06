﻿#include "Image/Loaders/LoaderPNG.h"
#include "Image/Loaders/LoaderPlatform.h"
#include "Math/Vector2.h"
#include "Math/Vector3.h"
#include "Range/Comparison/Equals.h"
#include "Image/AnyImage.h"
#include "Range/Polymorphic/InputRange.h"
#include "Cpp/Endianess.h"

namespace Intra { namespace Image {

bool LoaderPNG::IsValidHeader(const void* header, size_t headerSize) const
{
	const byte* headerBytes = reinterpret_cast<const byte*>(header);
	static const byte pngSignature[] = {137, 'P', 'N', 'G', 13, 10, 26, 10};
	return headerSize>=8 &&
		Equals(SpanOfPtr(headerBytes, sizeof(pngSignature)), SpanOf(pngSignature));
}

ImageInfo LoaderPNG::GetInfo(IInputStream& stream) const
{
	byte headerSignature[8];
	RawReadTo(stream, headerSignature, 8);
	if(!IsValidHeader(headerSignature, 8)) return ImageInfo();
	stream.PopFirstN(2*sizeof(intBE));

	const Math::UVec2 ihdrSize = Range::RawRead<Math::Vector2<uintBE>>(stream);
	const byte ihdrBitsPerComponent = Range::RawRead<byte>(stream);
	const byte ihdrColorType = Range::RawRead<byte>(stream);

	ImageFormat fmt = null;
	if(ihdrBitsPerComponent == 8)
	{
		if(ihdrColorType == 0) fmt = ImageFormat::Luminance8;
		else if(ihdrColorType == 4) fmt = ImageFormat::LuminanceAlpha8;
		else if(ihdrColorType == 2) fmt = ImageFormat::RGB8;
		else if(ihdrColorType == 6) fmt = ImageFormat::RGBA8;
	}
	else if(ihdrBitsPerComponent==16)
	{
		if(ihdrColorType == 0) fmt = ImageFormat::Luminance16;
		//else if(ihdrColorType == 4) fmt = ImageFormat::LuminanceAlpha16;
		else if(ihdrColorType == 2) fmt = ImageFormat::RGB16;
		else if(ihdrColorType == 6) fmt = ImageFormat::RGBA16;
	}
	return {
		Math::USVec3(ihdrSize.x, ihdrSize.y, 1),
		fmt, ImageType_2D, 0
	};
}

AnyImage LoaderPNG::Load(IInputStream& stream) const
{
#ifdef INTRA_USE_LIBPNG
	//TODO: сделать загрузку через libjpeg
#elif(INTRA_LIBRARY_IMAGE_LOADING!=INTRA_LIBRARY_IMAGE_LOADING_None)
	return LoadWithPlatform(stream);
#else
	(void)stream;
	return null;
#endif
}

const LoaderPNG LoaderPNG::Instance;

}}
