#include "pch.h"
#include "voicebasic.h"
#include "voicemale.h"
#include "voicefemale.h"
#include "tts.h"
#include "squirrel.h"
#include "dedicated.h"
#include "convar.h"

BasicVoice* b1 = new BasicVoice;
ConVar* Cvar_speechRate;

void say(const char* input, const char* voice)
{
	b1->skipSpeech();
	if (b1->speechRate != Cvar_speechRate->m_fValue) {
		b1->setRate(Cvar_speechRate->m_fValue);
	}
	b1->speakText(input);
}

// void function TTSsay()
SQRESULT SQ_TTSsay(void* sqvm)
{
	const SQChar* input = ClientSq_getstring(sqvm, 1);
	const SQChar* voice = ClientSq_getstring(sqvm, 2);

	say(input, voice);

	return SQRESULT_NULL;
}

void InitialiseTTS(HMODULE baseAddress)
{
	if (IsDedicated())
		return;

	Cvar_speechRate = RegisterConVar("speech_rate", "1", FCVAR_NONE, "");

	spdlog::info("adding tts func registration");
	g_ClientSquirrelManager->AddFuncRegistration("void", "TTSsay", "string input, string voice = \"M\"", "Say something using SAPI TTS", SQ_TTSsay);
	g_UISquirrelManager->AddFuncRegistration("void", "TTSsay", "string input, string voice = \"M\"", "Say something using SAPI TTS", SQ_TTSsay);
}