#include "pch.h"
#include "voicefemale.h"

void FemaleVoice::speakText(const char* input)
{
	if (SUCCEEDED(hr)) {
		wchar_t wtext[256];
		mbstowcs(wtext, (input), strlen(input) + 1);//Plus null
		LPWSTR ptr = wtext;
		hr = pVoice->Speak(ptr, 0, NULL);
	}
}

void FemaleVoice::outSpeech()
{
	pVoice->Release();
	pVoice = NULL;
	::CoUninitialize();
}