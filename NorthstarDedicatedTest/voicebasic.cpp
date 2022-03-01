#include "pch.h"
#include "voicebasic.h"

void BasicVoice::speakText(char* input)
{
	if (pVoice == NULL)
		return;
	size_t size = strlen(input) + 1;
	wchar_t* wtext = new wchar_t[size];

	size_t outSize;
	mbstowcs_s(&outSize, wtext, size, input, size - 1);
	LPWSTR ptr = wtext;
	hr = pVoice->Speak(ptr, SVSFlagsAsync + SVSFPurgeBeforeSpeak, NULL);
}

void BasicVoice::setRate(float rate)
{
	if (pVoice == NULL)
		return;
	speechRate = rate;
	pVoice->SetRate(rate);
}
void BasicVoice::setVolume(float volume)
{
	if (pVoice == NULL)
		return;
	speechVolume = volume;
	pVoice->SetVolume(volume * 100.0);
}
