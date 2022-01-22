#pragma once
#include "voicebasic.h"
class FemaleVoice :
	public BasicVoice
{
public:
	void speakText(const char* input);
	void outSpeech();
};