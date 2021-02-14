#include "SoundDriverInfo.h"

#if(defined(_WIN32) && defined(INTRA_DROP_XP_SUPPORT))

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS
#ifdef _MSC_VER
#pragma warning(disable: 4668) //'symbol' is not defined as a preprocessor macro, replacing with '0'
#endif

#include <initguid.h>
#include <Mmdeviceapi.h>
INTRA_WARNING_POP

#endif

namespace Intra { INTRA_BEGIN
#if(defined(_WIN32) && defined(INTRA_DROP_XP_SUPPORT))
SoundDeviceInfo SoundDeviceInfo::Get(bool* oSupported)
{
	SoundDeviceInfo result;

	if(oSupported) *oSupported = false;

	CoInitialize(nullptr);

	// get the device enumerator
	IMMDeviceEnumerator* pEnumerator = nullptr;
	if(FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
		CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&pEnumerator))))
		return result;

	// get default audio endpoint
	IMMDevice * pDevice = nullptr;
	if(FAILED(pEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &pDevice)))
		return result;

	IPropertyStore* store = nullptr;
	if(FAILED(pDevice->OpenPropertyStore(STGM_READ, &store)))
		return result;

	PROPVARIANT prop;
	if(FAILED(store->GetValue(PKEY_AudioEngine_DeviceFormat, &prop)))
		return result;

	PWAVEFORMATEX deviceFormatProperties;
	deviceFormatProperties = PWAVEFORMATEX(prop.blob.pBlobData);
	result.Channels = deviceFormatProperties->nChannels;
	result.SampleRate = deviceFormatProperties->nSamplesPerSec;
	result.BitDepth = deviceFormatProperties->wBitsPerSample;

	if(oSupported) *oSupported = true;

	return result;
}
#else

SoundDeviceInfo SoundDeviceInfo::Get(bool* oSupported)
{
	SoundDeviceInfo result;
	if(oSupported) *oSupported = false;
	return result;
}

#endif
} INTRA_END