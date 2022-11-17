#pragma once
// SplashScreen.h: interface for the CSplashScreen class.
//
//////////////////////////////////////////////////////////////////////

#include "resource1.h"

// Windows Header Files:
#include <windows.h>

#include <crtdbg.h> // for _ASSERT()
#include <stdio.h> // for vsprintf
#include <stdarg.h> // for vsprintf
#include <stdexcept>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <string>

#include "splash.h"

class NSSplashScreen
{
  public:
	NSSplashScreen();

	void HideSplashScreen();

	void SetSplashMessage(const char* message, int progress, bool close);

	BOOL RegisterClass(LPCTSTR szWindowClassName);

  protected:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	HFONT CreatePointFont(int pointSize, HDC dc);
	void Paint();

  private:
	int SPLASH_WIDTH = 512;
	int SPLASH_HEIGHT = 650;

	RECT redrawRect;
	RECT statusRect;
	HWND m_hDlg = NULL;
	HWND m_hParentWnd = NULL;
	HWND m_hWnd = NULL;
	HINSTANCE m_instance = NULL;
	static NSSplashScreen* m_pSplashWnd;
	static ATOM m_szWindowClass;
	static BOOL m_useStderr;

	HBITMAP m_bitmap = LoadBitmap(m_instance, MAKEINTRESOURCE(IDB_SPLASH));
	HBITMAP m_loadbar = LoadBitmap(m_instance, MAKEINTRESOURCE(IDB_LOAD));
	HBITMAP m_loadbar_filled = LoadBitmap(m_instance, MAKEINTRESOURCE(IDB_LOAD_FILLED));

	std::wstring m_message {};
	int m_progress = 0;
};

extern NSSplashScreen* g_SplashScreen;
extern "C" __declspec(dllexport) void SetSplashMessage(const char* msg, int progress, bool close = false);
