#pragma once
#include <sapi.h>
#include <iostream>
#include <string>
using namespace std;
class BasicVoice
{
  protected:
	ISpVoice* pVoice;
	HRESULT hr, a;

  public:
	float speechRate;
	float speechVolume;
	BasicVoice()
	{
		pVoice = NULL;
		a = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		if (!FAILED(a))
		{
			hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void**)&pVoice);
		}
	}
	virtual void speakText(char* input);
	virtual void setRate(float rate);
	virtual void setVolume(float volume);
	virtual ~BasicVoice()
	{
		::CoUninitialize();
		delete pVoice;
	}
};
