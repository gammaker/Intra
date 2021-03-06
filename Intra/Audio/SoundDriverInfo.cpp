#include "SoundDriverInfo.h"
#include "Cpp/PlatformDetect.h"

INTRA_PUSH_DISABLE_REDUNDANT_WARNINGS

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows && defined(INTRA_DROP_XP_SUPPORT))

#ifdef _MSC_VER
#pragma warning(disable: 4668)
#endif

#include <initguid.h>
#include <Mmdeviceapi.h>
#endif

namespace Intra { namespace Audio {

#if(INTRA_PLATFORM_OS == INTRA_PLATFORM_OS_Windows && defined(INTRA_DROP_XP_SUPPORT))
static SoundDeviceInfo Get(bool* oSupported)
{
	SoundDeviceInfo result;

	if(oSupported) *oSupported = false;

	CoInitialize(null);

	// get the device enumerator
	IMMDeviceEnumerator* pEnumerator = null;
	if(FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), null,
		CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&pEnumerator))))
		return result;

	// get default audio endpoint
	IMMDevice * pDevice = null;
	if(FAILED(pEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &pDevice)))
		return result;

	IPropertyStore* store = null;
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

SoundDeviceInfo Get(bool* oSupported)
{
	SoundDeviceInfo result;
	if(oSupported) *oSupported = false;
	return result;
}

#endif

}}

INTRA_WARNING_POP
