#pragma once
#include "voicebasic.h"
class MaleVoice :
	public BasicVoice
{
public:
	void speakText(const char* input);
	void outSpeech();
};
