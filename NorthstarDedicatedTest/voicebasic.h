#pragma once
#include <sapi.h>
#include<iostream>
#include <string>
using namespace std;
class BasicVoice
{
protected:
	int choice;
	ISpVoice* pVoice;
	HRESULT hr, a; /*In the field of computer programming, the HRESULT is a data type used in Windows operating systems, and the earlier IBM/Microsoft OS/2 operating system, to represent error conditions, and warning conditions.
The original purpose of HRESULTs was to formally lay out ranges of error codes for both public and Microsoft internal use in order to prevent collisions between error codes in different subsystems of the OS/2 operating system.
HRESULTs are numerical error codes. Various bits within an HRESULT encode information about the nature of the error code, and where it came from.
HRESULT error codes are most commonly encountered in COM programming, where they form the basis for a standardized COM error handling convention.*/
/*
For COM PROGRAMMING/INTERFACE
https://docs.microsoft.com/en-us/windows/win32/learnwin32/what-is-a-com-interface-
*/
	wstring input; /*string is a templated on a char, and wstring on a wchar_t.
				   it's a character type written wchar_t which is larger than the simple char character type.
				   It is supposed to be used to put inside characters whose indices (like Unicode glyphs) are larger than 255 (or 127, depending...).*/

public:
	BasicVoice() {
		pVoice = NULL; /* The ISpVoice interface enables an application to perform text synthesis operations.
		Applications can speak text strings and text files, or play audio files through this interface.
		All of these can be done synchronously or asynchronously.*/
		input = L"";
		a = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);//HRESULT CoInitializeEx(LPVOID pvReserved, DWORD dwCoInit);
		/*The first parameter is reserved and must be NULL.
		The second parameter specifies the threading model that your program will use.
		COM supports two different threading models, apartment threaded and multithreaded.
		If you specify apartment threading, you are making the following guarantees:
		You will access each COM object from a single thread; you will not share COM interface pointers between multiple threads.
		The thread will have a message loop
		Diff b/w appartment and multi threading https://stackoverflow.com/questions/485086/single-threaded-apartments-vs-multi-threaded-apartments*/
		if (FAILED(a))
		{
			cout << "ERROR 404 FAILED INITIALIZING COM\n";
			exit(1);
		}
		hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void**)&pVoice);
		/*
		The CoCreateInstance function provides a generic mechanism for creating objects.
		To understand CoCreateInstance, keep in mind that two COM objects can implement the same interface, and one object can implement two or more interfaces.
		Thus, a generic function that creates objects needs two pieces of information.
		Which object to create.
		Which interface to get from the object.
		HRESULT CoCreateInstance(
		REFCLSID  rclsid, //rclsid The CLSID associated with the data and code that will be used to create the object.
		LPUNKNOWN pUnkOuter, //If NULL, indicates that the object is not being created as part of an aggregate. If non-NULL, pointer to the aggregate object's IUnknown interface (the controlling IUnknown).
		DWORD     dwClsContext, //Context in which the code that manages the newly created object will run. The values are taken from the enumeration CLSCTX.
		REFIID    riid, //A reference to the identifier of the interface to be used to communicate with the object.
		LPVOID    *ppv //Address of pointer variable that receives the interface pointer requested in riid. Upon successful return, *ppv contains the requested interface pointer. Upon failure, *ppv contains NULL.
		last parameter explained in https://stackoverflow.com/questions/2941260/what-does-void-mean-in-c
		);
		*/
	}
	virtual void speakText(const char* input);
	virtual void outSpeech();
	virtual ~BasicVoice() {
		//cout << "Basic Voice Deleted\n";
		//pVoice = NULL;
		::CoUninitialize();
		delete pVoice;
	}
};