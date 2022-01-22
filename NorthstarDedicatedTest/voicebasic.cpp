#include "pch.h"
#include "voicebasic.h"

void BasicVoice::speakText(const char* input)
{
	if (SUCCEEDED(hr)) {
		wchar_t wtext[256];
		mbstowcs(wtext, (input), strlen(input) + 1);
		LPWSTR ptr = wtext;
		hr = pVoice->Speak(ptr, SVSFlagsAsync, NULL);
	}
}

void BasicVoice::outSpeech()
{
	thread speechThread([this]()
	{
		pVoice->Release();
		pVoice = NULL;
		::CoUninitialize();
	});
	speechThread.detach();
}