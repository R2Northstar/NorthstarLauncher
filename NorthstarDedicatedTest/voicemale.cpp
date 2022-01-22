#include "pch.h"
#include "voicemale.h"

void MaleVoice::speakText(const char* input)
{
	if (SUCCEEDED(hr)) {
		wchar_t wtext[256];
		mbstowcs(wtext, input, strlen(input) + 1);//Plus null
		LPWSTR ptr = wtext;
		hr = pVoice->Speak(ptr, SVSFlagsAsync, NULL);
	}
}

void MaleVoice::outSpeech()
{
	pVoice->Release();
	pVoice = NULL;
	::CoUninitialize();
}