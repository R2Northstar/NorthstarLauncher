#include "pch.h"
#include "voicebasic.h"
#include "tts.h"
#include "squirrel.h"
#include "dedicated.h"
#include "convar.h"

BasicVoice* b1 = new BasicVoice;
ConVar* Cvar_speechRate;
ConVar* Cvar_speechVolume;

char* input;
// void function TTSsay()
SQRESULT SQ_TTSsay(void* sqvm)
{
	const SQChar* inputSq = ClientSq_getstring(sqvm, 1);
	input = new char[strlen(inputSq) + 1];
	strcpy(input, inputSq);

	std::thread sayThread(
		[]()
		{
			// b1->skipSpeech();
			if (b1->speechRate != Cvar_speechRate->GetFloat())
			{
				b1->setRate(Cvar_speechRate->GetFloat());
			}
			if (b1->speechVolume != Cvar_speechRate->GetFloat())
			{
				b1->setVolume(Cvar_speechVolume->GetFloat());
			}
			b1->speakText(input);
		});
	sayThread.detach();

	return SQRESULT_NULL;
}

void InitialiseTTS(HMODULE baseAddress)
{
	if (IsDedicated())
		return;

	Cvar_speechRate = new ConVar("speech_rate", "2.5", FCVAR_NONE, "");
	Cvar_speechVolume = new ConVar("speech_volume", "0.5", FCVAR_NONE, "");

	spdlog::info("adding tts func registration");
	g_ClientSquirrelManager->AddFuncRegistration(
		"void", "TTSsay", "string input, string voice = \"M\"", "Say something using SAPI TTS", SQ_TTSsay);
	g_UISquirrelManager->AddFuncRegistration(
		"void", "TTSsay", "string input, string voice = \"M\"", "Say something using SAPI TTS", SQ_TTSsay);
}
