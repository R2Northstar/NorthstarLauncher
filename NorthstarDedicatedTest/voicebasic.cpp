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

void BasicVoice::setRate(float rate)
{
	speechRate = rate;
	pVoice->SetRate(rate);
}

void BasicVoice::outSpeech()
{
	pVoice->Release();
	pVoice = NULL;
	::CoUninitialize();
}

void BasicVoice::skipSpeech()
{
	unsigned long skipped;
	pVoice->Skip(L"SENTENCE", 1, &skipped);

	//spdlog::info("skipped {} sentences", skipped);
}