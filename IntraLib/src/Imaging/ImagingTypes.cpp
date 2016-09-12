﻿#include "Imaging/ImagingTypes.h"
#include "Containers/StringView.h"
#include "Containers/HashMap.h"

namespace Intra {

using namespace Math;

StringView ToString(ImageType t)
{
	INTRA_ASSERT(t<ImageType_End);
	static const StringView values[]={"1D", "1DArray", "2D", "2DArray", "3D", "Cube", "CubeArray"};
	INTRA_CHECK_TABLE_SIZE(values, ImageType_End);
	return values[(byte)t];
}

#pragma pack(push, 1)
struct FormatInfo
{
	ushort Type: 2; //0 - Invalid, 1 - normalized, 2 - floating point, 3 - integer
	ushort IsSigned: 1;
	ushort IsPacked: 1;
	ushort IsBasic: 1;
	ushort IsCompressed: 1;
	ushort HasLuminance: 1;
	ushort HasAlpha: 1;
	ushort HasDepth: 1;
	ushort HasStencil: 1;
	ushort ComponentCount: 2; //Число компонентов минус 1 (чтобы влезло в 2 бита)
	ushort HasSharedExponent: 1;
	ushort Is_sRGB: 1;
	byte BitsPerPixel; //Число битов на пиксель. Для сжатых форматов среднее
	ValueType::I DataType;
	ImageFormat::I BasicFormat;
	const char* Name;
};
#pragma pack(pop)



static FormatInfo get_format_info(ImageFormat fmt)
{
	static const FormatInfo uncompressedFormatsInfoTable[]=
	{
		{0,0,0,0,0,0,0,0,0,0,0,0,0,ValueType::Void, ImageFormat::None, "None"},

		/////////////////////
		//Unsigned normalized
		{1,0,0,0,0,0,0,0,0,0,0,0,8,ValueType::Norm8, ImageFormat::R, "Red8"},
		{1,0,0,0,0,1,0,0,0,0,0,0,8,ValueType::Norm8, ImageFormat::Luminance, "Luminance8"},
		{1,0,0,0,0,0,1,0,0,0,0,0,8,ValueType::Norm8, ImageFormat::Alpha, "Alpha8"},

		{1,0,0,0,0,0,0,0,0,1,0,0,16,ValueType::N8Vec2, ImageFormat::RG, "RG8"},
		{1,0,0,0,0,1,1,0,0,1,0,0,16,ValueType::N8Vec2, ImageFormat::LuminanceAlpha, "LuminanceAlpha8"},

		{1,0,0,0,0,0,0,0,0,2,0,0,24,ValueType::N8Vec3, ImageFormat::RGB, "RGB8"},
		{1,0,0,0,0,0,1,0,0,3,0,0,32,ValueType::N8Vec4, ImageFormat::RGBA, "RGBA8"},
		{1,0,0,0,0,0,0,0,0,2,0,0,32,ValueType::N8Vec4, ImageFormat::RGBA, "RGBX8"}, //альфа-канал не используется

		{1,0,0,0,0,0,0,0,0,0,0,0,16,ValueType::Norm16, ImageFormat::R, "Red16"},
		{1,0,0,0,0,1,0,0,0,0,0,0,16,ValueType::Norm16, ImageFormat::Luminance, "Luminance16"},
		{1,0,0,0,0,0,1,0,0,0,0,0,16,ValueType::Norm16, ImageFormat::Alpha, "Alpha16"},

		{1,0,0,0,0,0,0,0,0,1,0,0,32,ValueType::N16Vec2, ImageFormat::RG, "RG16"},
		{1,0,0,0,0,1,1,0,0,1,0,0,32,ValueType::N16Vec2, ImageFormat::LuminanceAlpha, "LuminanceAlpha16"},

		{1,0,0,0,0,0,0,0,0,2,0,0,48,ValueType::N16Vec3, ImageFormat::RGB, "RGB16"},
		{1,0,0,0,0,0,1,0,0,3,0,0,64,ValueType::N16Vec4, ImageFormat::RGBA, "RGBA16"},
		{1,0,0,0,0,0,0,0,0,2,0,0,64,ValueType::N16Vec4, ImageFormat::RGBA, "RGBX16"},

		{1,0,0,0,0,0,0,0,0,0,0,0,32,ValueType::Norm32, ImageFormat::R, "Red32"},
		{1,0,0,0,0,1,0,0,0,0,0,0,32,ValueType::Norm32, ImageFormat::Luminance, "Luminance32"},
		{1,0,0,0,0,0,1,0,0,0,0,0,32,ValueType::Norm32, ImageFormat::Alpha, "Alpha32"},

		{1,0,0,0,0,0,0,0,0,1,0,0,64,ValueType::N32Vec2, ImageFormat::RG, "RG32"},
		{1,0,0,0,0,1,1,0,0,1,0,0,64,ValueType::N32Vec2, ImageFormat::LuminanceAlpha, "LuminanceAlpha32"}, //2 компонента

		{1,0,0,0,0,0,0,0,0,2,0,0,96,ValueType::N32Vec3, ImageFormat::RGB, "RGB32"},
		{1,0,0,0,0,0,1,0,0,3,0,0,128,ValueType::N32Vec4, ImageFormat::RGBA, "RGBA32"},
		{1,0,0,0,0,0,0,0,0,2,0,0,128,ValueType::N32Vec4, ImageFormat::RGBA, "RGBX32"}, //альфа не используется

		/////////
		//Packed
		{1,0,1,0,0,0,0,0,0,2,0,0,8,ValueType::NVec233, ImageFormat::RGB, "BGR233"},
		{1,0,1,0,0,0,0,0,0,2,0,0,16,ValueType::NVec565, ImageFormat::RGB, "RGB565"},
		{1,0,1,0,0,0,1,0,0,3,0,0,16,ValueType::NVec1555, ImageFormat::RGBA, "A1_BGR5"},
		{1,0,1,0,0,0,0,0,0,2,0,0,16,ValueType::NVec1555, ImageFormat::RGBA, "X1_BGR5"},
		{1,0,1,0,0,0,1,0,0,3,0,0,16,ValueType::NVec4444, ImageFormat::RGBA, "ABGR4"},
		{1,0,1,0,0,0,0,0,0,2,0,0,16,ValueType::NVec4444, ImageFormat::RGBA, "XBGR4"},
		{1,0,1,0,0,0,1,0,0,3,0,0,32,ValueType::Vec10n10n10n2n, ImageFormat::RGBA, "RGB10A2"},
		{1,0,1,0,0,0,0,0,0,2,0,0,32,ValueType::N10Vec3, ImageFormat::RGB, "RGB10"},
		{1,0,1,0,0,0,0,0,0,2,0,0,16,ValueType::N5Vec3, ImageFormat::RGB, "RGB5"},
		{1,0,1,0,0,0,1,0,0,3,0,0,16,ValueType::NVec5551, ImageFormat::RGBA, "RGB5A1"},
		{1,0,1,0,0,0,1,0,0,3,0,0,16,ValueType::NVec4444, ImageFormat::RGBA, "RGBA4"},
		{1,0,1,0,0,0,0,0,0,2,0,0,16,ValueType::NVec4444, ImageFormat::RGBA, "RGBX4"},


		///////////////////
		//Signed normalized

		//8 bit
		{1,1,0,0,0,0,0,0,0,0,0,0,8,ValueType::SNorm8, ImageFormat::Rs, "Red8s"},
		{1,1,0,0,0,0,0,0,0,1,0,0,16,ValueType::S8Vec2, ImageFormat::RGs, "RG8s"},
		{1,1,0,0,0,0,0,0,0,2,0,0,24,ValueType::S8Vec3, ImageFormat::RGBs, "RGB8s"},
		{1,1,0,0,0,0,1,0,0,3,0,0,32,ValueType::S8Vec4, ImageFormat::RGBAs, "RGBA8s"},

		//16 bit
		{1,1,0,0,0,0,0,0,0,0,0,0,16,ValueType::SNorm16, ImageFormat::Rs, "Red16s"},
		{1,1,0,0,0,0,0,0,0,1,0,0,32,ValueType::S16Vec2, ImageFormat::RGs, "RG16s"},
		{1,1,0,0,0,0,0,0,0,2,0,0,48,ValueType::S16Vec3, ImageFormat::RGBs, "RGB16s"},
		{1,1,0,0,0,0,1,0,0,3,0,0,64,ValueType::S16Vec4, ImageFormat::RGBAs, "RGBA16s"},

		//32 bit
		{1,1,0,0,0,0,0,0,0,0,0,0,32,ValueType::SNorm32, ImageFormat::Rs, "Red32s"},
		{1,1,0,0,0,0,0,0,0,1,0,0,64,ValueType::S32Vec2, ImageFormat::RGs, "RG32s"},
		{1,1,0,0,0,0,0,0,0,2,0,0,96,ValueType::S32Vec3, ImageFormat::RGBs, "RGB32s"},
		{1,1,0,0,0,0,1,0,0,3,0,0,128,ValueType::S32Vec4, ImageFormat::RGBAs, "RGBA32s"},

		//sRGB
		{1,0,0,0,0,0,0,0,0,2,0,1,24,ValueType::N8Vec3, ImageFormat::sRGB, "sRGB8"},
		{1,0,0,0,0,0,1,0,0,3,0,1,32,ValueType::N8Vec4, ImageFormat::sRGB_A, "sRGB8_A8"},

		////////////////
		//Floating point

		//FP16
		{2,1,0,0,0,0,0,0,0,0,0,0,16,ValueType::Half, ImageFormat::Rf, "Red16f"},
		{2,1,0,0,0,1,0,0,0,0,0,0,16,ValueType::Half, ImageFormat::Luminancef, "Luminance16f"},
		{2,1,0,0,0,0,0,0,0,1,0,0,32,ValueType::HVec2, ImageFormat::RGf, "RG16f"},
		{2,1,0,0,0,0,0,0,0,2,0,0,48,ValueType::HVec3, ImageFormat::RGBf, "RGB16f"},
		{2,1,0,0,0,0,1,0,0,3,0,0,64,ValueType::HVec4, ImageFormat::RGBAf, "RGBA16f"},

		//FP32
		{2,1,0,0,0,0,0,0,0,0,0,0,32,ValueType::Float, ImageFormat::Rf, "Red32f"},
		{2,1,0,0,0,1,0,0,0,0,0,0,32, ValueType::Float, ImageFormat::Luminancef, "Luminance32f"},

		{2,1,0,0,0,0,0,0,0,1,0,0,64,ValueType::FVec2, ImageFormat::RGf, "RG32f"},
		{2,1,0,0,0,0,0,0,0,2,0,0,96,ValueType::FVec3, ImageFormat::RGBf, "RGB32f"},
		{2,1,0,0,0,0,1,0,0,3,0,0,128,ValueType::FVec4, ImageFormat::RGBAf, "RGBA32f"},

		//Packed
		{2,0,1,0,0,0,0,0,0,2,0,0,32,ValueType::Vec11f11f10f, ImageFormat::RGBf, "R11G11B10f"},
		{2,0,1,0,0,0,0,0,0,3,1,0,32,ValueType::UVec9995, ImageFormat::RGBf, "RGB9E5"},
		{2,0,0,0,0,0,0,0,0,3,1,0,32,ValueType::UBVec4, ImageFormat::RGBf, "RGBE8"},



		//////////////////
		//Unsigned integer

		//8 bit
		{3,0,0,0,0,0,0,0,0,0,0,0,8,ValueType::Byte, ImageFormat::Ru, "Red8u"},
		{3,0,0,0,0,0,0,0,0,1,0,0,16,ValueType::UBVec2, ImageFormat::RGu, "RG8u"},
		{3,0,0,0,0,0,0,0,0,2,0,0,24, ValueType::UBVec3, ImageFormat::RGBu, "RGB8u"},
		{3,0,0,0,0,0,1,0,0,3,0,0,32,ValueType::UBVec4, ImageFormat::RGBAu, "RGBA8u"},

		//16 bit
		{3,0,0,0,0,0,0,0,0,0,0,0,16,ValueType::UShort, ImageFormat::Ru, "Red16u"},
		{3,0,0,0,0,0,0,0,0,1,0,0,32,ValueType::USVec2, ImageFormat::RGu, "RG16u"},
		{3,0,0,0,0,0,0,0,0,2,0,0,48,ValueType::USVec3, ImageFormat::RGBu, "RGB16u"},
		{3,0,0,0,0,0,1,0,0,3,0,0,64,ValueType::USVec4, ImageFormat::RGBAu, "RGBA16u"},


		//32 bit
		{3,0,0,0,0,0,0,0,0,0,0,0,32,ValueType::UInt, ImageFormat::Ru, "Red32u"},
		{3,0,0,0,0,0,0,0,0,1,0,0,64,ValueType::UVec2, ImageFormat::RGu, "RG32u"},
		{3,0,0,0,0,0,0,0,0,2,0,0,96,ValueType::UVec3, ImageFormat::RGBu, "RGB32u"},
		{3,0,0,0,0,0,1,0,0,3,0,0,128,ValueType::UVec4, ImageFormat::RGBAu, "RGBA32u"},

		{3,0,1,0,0,0,0,0,0,3,0,0,32,ValueType::Vec10u10u10u2u, ImageFormat::RGBAu, "RGB10A2u"}, //Упакованные

		/////////////////
		//Signed integer

		//8 bit
		{3,1,0,0,0,0,0,0,0,0,0,0,8,ValueType::SByte, ImageFormat::Ri, "Red8i"},
		{3,1,0,0,0,0,0,0,0,1,0,0,16, ValueType::SBVec2, ImageFormat::RGi, "RG8i"},
		{3,1,0,0,0,0,0,0,0,2,0,0,24, ValueType::SBVec3, ImageFormat::RGBi, "RGB8i"},
		{3,1,0,0,0,0,1,0,0,3,0,0,32, ValueType::SBVec4, ImageFormat::RGBAi, "RGBA8i"},

		//16 bit
		{3,1,0,0,0,0,0,0,0,0,0,0,16,ValueType::Short, ImageFormat::Ri, "Red16i"},
		{3,1,0,0,0,0,0,0,0,1,0,0,32,ValueType::SVec2, ImageFormat::RGi, "RG16i"},
		{3,1,0,0,0,0,0,0,0,2,0,0,48,ValueType::SVec3, ImageFormat::RGBi, "RGB16i"},
		{3,1,0,0,0,0,1,0,0,3,0,0,64,ValueType::SVec4, ImageFormat::RGBAi, "RGBA16i"},

		//32 bit
		{3,1,0,0,0,0,0,0,0,0,0,0,32,ValueType::Int, ImageFormat::Ri, "Red32i"},
		{3,1,0,0,0,0,0,0,0,1,0,0,64,ValueType::IVec2, ImageFormat::RGi, "RG32i"},
		{3,1,0,0,0,0,0,0,0,2,0,0,96,ValueType::IVec3, ImageFormat::RGBi, "RGB32i"},
		{3,1,0,0,0,0,1,0,0,3,0,0,128,ValueType::IVec4, ImageFormat::RGBAi, "RGBA32i"},

		//Depth\Stencil
		{1,0,0,0,0,0,0,1,0,0,0,0,16,ValueType::UShort, ImageFormat::Depth, "Depth16"},
		{1,0,0,0,0,0,0,1,0,0,0,0,24,ValueType::UInt, ImageFormat::Depth, "Depth24"},
		{1,0,0,0,0,0,0,1,1,1,0,0,32,ValueType::UInt, ImageFormat::DepthStencil, "Depth24Stencil8"},
		{1,0,0,0,0,0,0,1,0,0,0,0,32,ValueType::UInt, ImageFormat::Depth, "Depth32"},
		{2,0,0,0,0,0,0,1,0,0,0,0,32,ValueType::Float, ImageFormat::DepthF, "Depth32f"},
		{2,0,0,0,0,0,0,1,1,1,0,0,40,ValueType::Void, ImageFormat::DepthF_Stencil, "Depth32fStencil8"}
	};
	INTRA_CHECK_TABLE_SIZE(uncompressedFormatsInfoTable, ImageFormat::EndOfUncompressed);

	static const FormatInfo compressedFormatsInfoTable[]=
	{
		////////////
		//DXT
		{1,0,0,0,1,0,0,0,0,2,0,0,4,ValueType::Void, ImageFormat::RGB,  "DXT1_RGB"},
		{1,0,0,0,1,0,1,0,0,3,0,0,4,ValueType::Void, ImageFormat::RGBA, "DXT1_RGBA"},
		{1,0,0,0,1,0,1,0,0,3,0,0,8,ValueType::Void, ImageFormat::RGBA, "DXT3_RGBA"},
		{1,0,0,0,1,0,1,0,0,3,0,0,8,ValueType::Void, ImageFormat::RGBA, "DXT5_RGBA"},
		{1,0,0,0,1,0,0,0,0,2,0,1,4,ValueType::Void, ImageFormat::sRGB,   "DXT1_sRGB"},
		{1,0,0,0,1,0,1,0,0,3,0,1,4,ValueType::Void, ImageFormat::sRGB_A, "DXT1_sRGB_A"},
		{1,0,0,0,1,0,1,0,0,3,0,1,8,ValueType::Void, ImageFormat::sRGB_A, "DXT3_sRGB_A"},
		{1,0,0,0,1,0,1,0,0,3,0,1,8,ValueType::Void, ImageFormat::sRGB_A, "DXT5_sRGB_A"},

		///////////
		//ETC
		{1,0,0,0,1,0,0,0,0,2,0,0,4,ValueType::Void, ImageFormat::RGB, "ETC1_RGB"},
		{1,0,0,0,1,0,0,0,0,0,0,0,4,ValueType::Void, ImageFormat::R,  "EAC_Red"},
		{1,0,0,0,1,0,0,0,0,1,0,0,8,ValueType::Void, ImageFormat::RG, "EAC_RG"},
		{1,1,0,0,1,0,0,0,0,0,0,0,4,ValueType::Void, ImageFormat::Rs,  "EAC_Rs"},
		{1,1,0,0,1,0,0,0,0,1,0,0,8,ValueType::Void, ImageFormat::RGs, "EAC_RGs"},
		{1,0,0,0,1,0,0,0,0,2,0,0,4,ValueType::Void, ImageFormat::RGB,  "ETC2_RGB"},
		{1,0,0,0,1,0,0,0,0,2,0,1,4,ValueType::Void, ImageFormat::sRGB, "ETC2_sRGB"},
		{1,0,0,0,1,0,1,0,0,3,0,0,8,ValueType::Void, ImageFormat::RGBA, "ETC2_RGBA"},
		{1,0,0,0,1,0,1,0,0,3,0,0,8,ValueType::Void, ImageFormat::RGBA, "ETC2_RGB_BinAlpha"},
		{1,0,0,0,1,0,1,0,0,3,0,1,8,ValueType::Void, ImageFormat::sRGB, "ETC2_sRGB_BinAlpha"},

		///////////
		//ATC
		{1,0,0,0,1,0,0,0,0,2,0,0,4,ValueType::Void, ImageFormat::RGB,  "ATC_RGB"},
		{1,0,0,0,1,0,1,0,0,3,0,0,8,ValueType::Void, ImageFormat::RGBA, "ATC_RGB_ExplicitAlpha"},
		{1,0,0,0,1,0,1,0,0,3,0,0,8,ValueType::Void, ImageFormat::RGBA, "ATC_RGB_InterpolatedAlpha"},

		///////////
		//BPTC
		{1,0,0,0,1,0,0,0,0,2,0,0,8,ValueType::Void, ImageFormat::RGBA, "BPTC_RGBA"},
		{1,0,0,0,1,0,0,0,0,2,0,1,8,ValueType::Void, ImageFormat::sRGB_A, "BPTC_sRGB_A"},
		{2,1,0,0,1,0,0,0,0,2,0,0,8,ValueType::Void, ImageFormat::RGBf, "BPTC_RGBf"},
		{2,0,0,0,1,0,0,0,0,2,0,0,8,ValueType::Void, ImageFormat::RGBf, "BPTC_RGBuf"},

		//////////////
		//RGTC
		{1,0,0,0,1,0,0,0,0,0,0,0,4,ValueType::Void, ImageFormat::R, "RGTC_Red"},
		{1,1,0,0,1,0,0,0,0,0,0,0,4,ValueType::Void, ImageFormat::Rs, "RGTC_SignedRed"},
		{1,0,0,0,1,0,0,0,0,1,0,0,8,ValueType::Void, ImageFormat::RG, "RGTC_RG"},
		{1,1,0,0,1,0,0,0,0,1,0,0,8,ValueType::Void, ImageFormat::RGs, "RGTC_SignedRG"},

		/////////////
		//LATC
		{1,0,0,0,1,1,0,0,0,0,0,0,4,ValueType::Void, ImageFormat::Luminance, "LATC_Luminance"},
		{1,1,0,0,1,1,0,0,0,0,0,0,4,ValueType::Void, ImageFormat::Luminance, "LATC_SignedLuminance"},
		{1,0,0,0,1,1,1,0,0,1,0,0,8,ValueType::Void, ImageFormat::LuminanceAlpha, "LATC_LuminanceAlpha"},
		{1,1,0,0,1,1,1,0,0,1,0,0,8,ValueType::Void, ImageFormat::LuminanceAlpha, "LATC_SignedLuminanceAlpha"},

		/////////////
		//PVRTC
		{1,0,0,0,1,0,0,0,0,2,0,0,4,ValueType::Void, ImageFormat::RGB, "PVRTC_RGB_4bpp"},
		{1,0,0,0,1,0,1,0,0,3,0,0,2,ValueType::Void, ImageFormat::RGB, "PVRTC_RGB_2bpp"},
		{1,0,0,0,1,0,1,0,0,3,0,0,4,ValueType::Void, ImageFormat::RGBA, "PVRTC_RGBA_4bpp"},
		{1,0,0,0,1,0,0,0,0,2,0,0,2,ValueType::Void, ImageFormat::RGBA, "PVRTC_RGBA_2bpp"},
	};
	INTRA_CHECK_TABLE_SIZE(compressedFormatsInfoTable, ImageFormat::EndOfCompressed-ImageFormat::FirstOfCompressed);

	static const FormatInfo basicFormatsInfoTable[]=
	{
		//Unsigned normalized
		{1,0,0,1,0,0,0,0,0,0,0,0,0,ValueType::Void, ImageFormat::R, "R"},
		{1,0,0,1,0,1,0,0,0,0,0,0,0,ValueType::Void, ImageFormat::Luminance, "Luminance"},
		{1,0,0,1,0,0,1,0,0,0,0,0,0,ValueType::Void, ImageFormat::Alpha, "Alpha"},
		{1,0,0,1,0,0,0,0,0,1,0,0,0,ValueType::Void, ImageFormat::LuminanceAlpha, "LuminanceAlpha"},
		{1,0,0,1,0,1,1,0,0,1,0,0,0,ValueType::Void, ImageFormat::RG, "RG"},
		{1,0,0,1,0,0,0,0,0,2,0,0,0,ValueType::Void, ImageFormat::RGB, "RGB"},
		{1,0,0,1,0,0,1,0,0,3,0,0,0,ValueType::Void, ImageFormat::RGBA, "RGBA"},

		//Signed normalized
		{1,1,0,1,0,0,0,0,0,0,0,0,0,ValueType::Void, ImageFormat::Rs, "Rs"},
		{1,1,0,1,0,0,0,0,0,1,0,0,0,ValueType::Void, ImageFormat::RGs, "RGs"},
		{1,1,0,1,0,0,0,0,0,2,0,0,0,ValueType::Void, ImageFormat::RGBs, "RGBs"},
		{1,1,0,1,0,0,1,0,0,3,0,0,0,ValueType::Void, ImageFormat::RGBAs, "RGBAs"},

		//Unsigned integer
		{3,0,0,1,0,0,0,0,0,0,0,0,0,ValueType::Void, ImageFormat::Ru, "Ru"},
		{3,0,0,1,0,0,0,0,0,1,0,0,0,ValueType::Void, ImageFormat::RGu, "RGu"},
		{3,0,0,1,0,0,0,0,0,2,0,0,0,ValueType::Void, ImageFormat::RGBu, "RGBu"},
		{3,0,0,1,0,0,1,0,0,3,0,0,0,ValueType::Void, ImageFormat::RGBAu, "RGBAu"},

		//Signed integer
		{3,1,0,1,0,0,0,0,0,0,0,0,0,ValueType::Void, ImageFormat::Ri, "Ri"},
		{3,1,0,1,0,0,0,0,0,1,0,0,0,ValueType::Void, ImageFormat::RGi, "RGi"},
		{3,1,0,1,0,0,0,0,0,2,0,0,0,ValueType::Void, ImageFormat::RGBi, "RGBi"},
		{3,1,0,1,0,0,1,0,0,3,0,0,0,ValueType::Void, ImageFormat::RGBAi, "RGBAi"},

		//Floating point
		{2,1,0,1,0,0,0,0,0,0,0,0,0,ValueType::Void, ImageFormat::Rf, "Rf"},
		{2,1,0,1,0,1,0,0,0,0,0,0,0,ValueType::Void, ImageFormat::Luminancef, "Luminancef"},
		{2,1,0,1,0,0,0,0,0,1,0,0,0,ValueType::Void, ImageFormat::RGf, "RGf"},
		{2,1,0,1,0,0,0,0,0,2,0,0,0,ValueType::Void, ImageFormat::RGBf, "RGBf"},
		{2,1,0,1,0,0,1,0,0,3,0,0,0,ValueType::Void, ImageFormat::RGBAf, "RGBAf"},

		//sRGB
		{1,0,0,1,0,0,0,0,0,2,0,1,0,ValueType::Void, ImageFormat::sRGB, "sRGB"},
		{1,0,0,1,0,0,1,0,0,3,0,1,0,ValueType::Void, ImageFormat::sRGB_A, "sRGB_A"},

		//Depth
		{1,0,0,1,0,0,0,1,0,0,0,0,0,ValueType::Void, ImageFormat::Depth, "Depth"},
		{2,0,0,1,0,0,0,1,0,0,0,0,0,ValueType::Void, ImageFormat::DepthF, "DepthF"},
		{1,0,0,1,0,0,0,1,1,1,0,0,0,ValueType::Void, ImageFormat::DepthStencil, "DepthStencil"},
		{2,0,0,1,0,0,0,1,1,1,0,0,0,ValueType::Void, ImageFormat::DepthF_Stencil, "DepthF_Stencil"}
	};
	INTRA_CHECK_TABLE_SIZE(basicFormatsInfoTable, ImageFormat::EndOfBasic-ImageFormat::FirstOfBasic);

	if(!fmt.IsValid()) return {0,0,0,0,0,0,0,0,0,0,0,0,0, ValueType::Void, ImageFormat::None, ""};
	if(fmt.value<ImageFormat::EndOfUncompressed)
		return uncompressedFormatsInfoTable[fmt.value];

	if(fmt.value>=ImageFormat::FirstOfCompressed && fmt.value<ImageFormat::EndOfCompressed)
		return compressedFormatsInfoTable[fmt.value-ImageFormat::FirstOfCompressed];

	if(fmt.value>=ImageFormat::FirstOfBasic && fmt.value<ImageFormat::EndOfBasic)
		return basicFormatsInfoTable[fmt.value-ImageFormat::FirstOfBasic];

	INTRA_ASSERT(!"Error in get_format_info!");
	return {0,0,0,0,0,0,0,0,0,0,0,0,0,ValueType::Void, ImageFormat::None, ""};
}

ImageFormat ImageFormat::GetBasicFormat() const
{
	return get_format_info(*this).BasicFormat;
}

byte ImageFormat::BitsPerPixel() const
{
	return get_format_info(*this).BitsPerPixel;
}

byte ImageFormat::BitsPerComponent() const
{
	auto fi = get_format_info(*this);
	return (byte)(fi.BitsPerPixel/(fi.ComponentCount+1));
}

byte ImageFormat::ComponentCount() const
{
	auto info = get_format_info(*this);
	return byte(info.Type==0? 0: info.ComponentCount+1);
}

bool ImageFormat::IsValid() const
{
	return ((value>=FirstOfUncompressed && value<EndOfUncompressed) ||
		(value>=FirstOfCompressed && value<EndOfCompressed) ||
		(value>=FirstOfBasic && value<EndOfBasic));
}

bool ImageFormat::IsNormalized() const {return get_format_info(*this).Type==1;}
bool ImageFormat::IsCompressed() const {return value>=FirstOfCompressed && value<EndOfCompressed;}

bool ImageFormat::IsCompressedBC1_BC7() const
{
	return (value>=DXT1_RGB && value<=DXT5_sRGB_A) ||
		(value>=RGTC_Red && value<=LATC_SignedLuminanceAlpha) ||
		(value>=BPTC_RGBA && value<=BPTC_RGBuf);
}

bool ImageFormat::IsFloatingPoint() const {return get_format_info(*this).Type==2;}
bool ImageFormat::IsIntegral() const      {return get_format_info(*this).Type==3;}
bool ImageFormat::IsSigned() const        {return get_format_info(*this).IsSigned;}
bool ImageFormat::IsBasic() const         {return get_format_info(*this).IsBasic;}
bool ImageFormat::HasDepth() const        {return get_format_info(*this).HasDepth;}
bool ImageFormat::HasStencil() const      {return get_format_info(*this).HasStencil;}
bool ImageFormat::IsLuminance() const     {auto info=get_format_info(*this); return info.HasLuminance && info.ComponentCount==0;}
bool ImageFormat::HasLuminance() const    {return get_format_info(*this).HasLuminance;}
bool ImageFormat::IsAlpha() const         {auto info=get_format_info(*this); return info.HasAlpha && info.ComponentCount==0;}
bool ImageFormat::HasAlpha() const        {return get_format_info(*this).HasAlpha;}

bool ImageFormat::HasColor() const
{
	auto info = get_format_info(*this);
	return !info.HasLuminance && !info.HasDepth && !info.HasStencil;
}

bool ImageFormat::IsPacked() const  {return get_format_info(*this).IsPacked;}
ValueType ImageFormat::GetValueType() const
{
	return value<EndOfUncompressed? get_format_info(*this).DataType: ValueType::Void;
}

ValueType ImageFormat::GetComponentType() const {return GetValueType().ToScalarType();}
StringView ImageFormat::ToString() const {return StringView(get_format_info(*this).Name);}

ImageFormat ImageFormat::FromString(StringView str, bool* exists)
{
	static HashMap<StringView, ImageFormat> formatTable;
	if(formatTable==null)
	{
		for(ushort i=0; i<ImageFormat::FirstOfCompressed; i++)
			formatTable[ImageFormat(i).ToString()] = ImageFormat(i);
		for(ushort i=ImageFormat::FirstOfCompressed; i<ImageFormat::EndOfCompressed; i++)
			formatTable[ImageFormat(i).ToString()] = ImageFormat(i);
		for(ushort i=ImageFormat::FirstOfBasic; i<ImageFormat::EndOfBasic; i++)
			formatTable[ImageFormat(i).ToString()] = ImageFormat(i);
	}
	auto result = formatTable.Get(str, exists);

	return result;
}

UVec4 ImageFormat::GetBitMasks(bool swapRB) const
{
	static const UVec4 formatBitMasks[]=
	{
		{0xFF, 0, 0, 0}, {0xFF, 0xFF, 0xFF, 0}, {0, 0, 0, 0xFF}, {0xFF, 0xFF00, 0, 0}, {0xFF, 0xFF, 0xFF, 0xFF00},
		{0xFF, 0xFF00, 0xFF0000, 0}, {0xFF, 0xFF00, 0xFF0000, 0xFF000000}, {0xFF, 0xFF00, 0xFF0000, 0},

		{0xFFFF, 0, 0, 0}, {0xFFFF, 0xFFFF, 0xFFFF, 0}, {0, 0, 0, 0xFFFF},
		{0xFFFF, 0xFFFF0000, 0, 0}, {0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF0000},
		{0,0,0,0}, {0,0,0,0}, {0,0,0,0},

		{0xFFFFFFFF, 0, 0, 0}, {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0}, {0, 0, 0, 0xFFFFFFFF},
		{0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0},

		{0xE0, 0x1C, 3, 0}, {0x1F, 0x7E0, 0xF800, 0}, {0xF800, 0x7C0, 0x3E, 1},
		{0xF800, 0x7C0, 0x3E, 0}, {0xF000, 0xF00, 0xF0, 0xF}, {0xF000, 0xF00, 0xF0, 0},
		{0x3FF, 0xFFC00, 0x3FF00000, 0xC0000000}, {0x3FF, 0xFFC00, 0x3FF00000, 0},
		{0x1F, 0x3E0, 0x7C00, 0}, {0x1F, 0x3E0, 0x7C00, 0x8000}, {0xF, 0xF0, 0xF00, 0xF000}, {0xF, 0xF0, 0xF00, 0}
	};
	INTRA_CHECK_TABLE_SIZE(formatBitMasks, RGBX4+1-Red8);

	if(value<Red8 || value>RGBX4) return {0,0,0,0}; //Битовые маски определены только для несжатых нормализованных беззнаковых форматов, число бит на пиксель которых не больше 32
	if(swapRB) return formatBitMasks[value-Red8].swizzle(2,1,0,3);
	return formatBitMasks[value-Red8];
}




ushort ImageInfo::CalculateMaxMipmapCount() const
{
	ushort maxDimension = Max(Size.x, Size.y);
	if(Type!=ImageType_2DArray) maxDimension = Max(maxDimension, Size.z);
	const short maxUncompressedLevel = maxDimension==1? 0: Log2i(Max(1u, (uint)maxDimension));
	ushort numLevels = ushort(maxUncompressedLevel+1);
	return Max(numLevels, (ushort)0);
}

USVec3 CalculateMipmapSize(USVec3 size, ImageType type, uint mip)
{
	USVec3 result;
	result.z=size.z;
	const ushort dims = 2+(type==ImageType_3D);
	for(ushort i=0; i<dims; i++)
		result[i] = Max<ushort>(1, ushort(size[i] >> mip));
	return result;
}

USVec3 CalculateMipmapOffset(USVec3 offset, ImageType type, uint mip)
{
	USVec3 result = CalculateMipmapSize(offset, type, mip);
	return Min(result, USVec3(offset));
}

USVec3 ImageInfo::CalculateMipmapSize(uint mip) const
{
	if(mip>=MipmapCount && mip!=0) return {0,0,0};
	return Intra::CalculateMipmapSize(Size, Type, mip);
}

size_t ImageInfo::CalculateMipmapDataSize(uint mip, uint lineAlignment) const
{
	Math::USVec3 sz = CalculateMipmapSize(mip);
	const ushort dims = 2+(Type==ImageType_3D);
	uint alignmentBytes = 0;
	const uint bpp = Format.BitsPerPixel();
	if(!Format.IsCompressed())
		alignmentBytes = lineAlignment-1-(sz.x*bpp/8+lineAlignment-1)%lineAlignment;
	else for(uint k=0; k<dims; k++)
		if(sz[k]%4!=0) sz[k] = short((sz[k]/4+1)*4);
	return (sz.x*bpp/8+alignmentBytes)*sz.y*sz.z;
}

size_t ImageInfo::CalculateFullDataSize(uint lineAlignment) const
{
	size_t size=0;
	for(ushort i=0, mips=MipmapCount==0? 1: MipmapCount; i<mips; i++)
		size += CalculateMipmapDataSize(i, lineAlignment);
	return size;
}

bool ImageFormat::IsSRGB() const {return get_format_info(*this).Is_sRGB!=0;}

ImageFormat ImageFormat::ToSRGB() const
{
	auto info = get_format_info(*this);
	if(info.Is_sRGB || info.Type!=1 || 1+info.ComponentCount<3) return *this;
	if(!info.IsCompressed)
	{
		if(value==RGB) return sRGB;
		if(value==RGB8) return sRGB8;
		if(value==RGBA8) return sRGB8_A8;
		if(value==RGBX8) return sRGB8_A8;
	}
	else
	{
		if(value==DXT1_RGB) return DXT1_sRGB;
		if(value==DXT1_RGBA) return DXT1_sRGB_A;
		if(value==DXT3_RGBA) return DXT3_sRGB_A;
		if(value==DXT5_RGBA) return DXT5_sRGB_A;
		if(value==ETC2_RGB) return ETC2_sRGB;
		if(value==ETC2_RGB_BinAlpha) return ETC2_sRGB_BinAlpha;
		if(value==BPTC_RGBA) return BPTC_sRGB_A;
	}
	return *this;
}

ImageFormat ImageFormat::ToNonSRGB() const
{
	auto info = get_format_info(*this);
	if(!info.Is_sRGB || info.Type!=1 || 1+info.ComponentCount<3) return *this;
	if(!info.IsCompressed)
	{
		if(value==sRGB) return RGB;
		if(value==sRGB8) return RGB8;
		if(value==sRGB8_A8) return RGBA8;
	}
	else
	{
		if(value==DXT1_sRGB) return DXT1_RGB;
		if(value==DXT1_sRGB_A) return DXT1_RGBA;
		if(value==DXT3_sRGB_A) return DXT3_RGBA;
		if(value==DXT5_sRGB_A) return DXT5_RGBA;
		if(value==ETC2_sRGB) return ETC2_RGB;
		if(value==ETC2_sRGB_BinAlpha) return ETC2_RGB_BinAlpha;
		if(value==BPTC_sRGB_A) return BPTC_RGBA;
	}
	return *this;
}

}