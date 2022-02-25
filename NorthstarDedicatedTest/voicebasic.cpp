#include "pch.h"
#include "voicebasic.h"

void BasicVoice::speakText(char* input)
{
	size_t size = strlen(input) + 1;
	wchar_t* wtext = new wchar_t[size];

	size_t outSize;
	mbstowcs_s(&outSize, wtext, size, input, size - 1);
	LPWSTR ptr = wtext;
	hr = pVoice->Speak(ptr, SVSFlagsAsync + SVSFPurgeBeforeSpeak, NULL);
}

void BasicVoice::setRate(float rate)
{
	speechRate = rate;
	pVoice->SetRate(rate);
}
void BasicVoice::setVolume(float volume)
{
	speechVolume = volume;
	pVoice->SetVolume(volume);
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
}
