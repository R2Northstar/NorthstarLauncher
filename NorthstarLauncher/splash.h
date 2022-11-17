#pragma once
// SplashScreen.h: interface for the CSplashScreen class.
//
//////////////////////////////////////////////////////////////////////

#include "resource1.h"

// Windows Header Files:
#include <windows.h>
#include <stdexcept>

// C RunTime Header Files
#include <string>

#include "splash.h"

class NSSplashScreen
{
  public:
	NSSplashScreen(std::string altBackground = "");

	void HideSplashScreen();

	void SetSplashMessage(const char* message, int progress, bool close);

  protected:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	HFONT CreatePointFont(int pointSize, HDC dc);
	BOOL RegisterClass(LPCTSTR szWindowClassName);
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

	bool m_useAltBackground = false;
	std::wstring m_altBackground {};

	std::wstring m_message {};
	int m_progress = 0;
};

extern NSSplashScreen* g_SplashScreen;
extern "C" __declspec(dllexport) void SetSplashMessage(const char* msg, int progress, bool close = false);
