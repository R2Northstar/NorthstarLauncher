#pragma once
// SplashScreen.h: interface for the CSplashScreen class.
//
//////////////////////////////////////////////////////////////////////

#include "resource1.h"

// Windows Header Files:
#include <windows.h>
#include <stdexcept>
#include <thread>

// C RunTime Header Files
#include <string>

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

	void LoadDefaults();
	void LoadFromFile(std::string& path);

  private:
	int SPLASH_WIDTH = 512;
	int SPLASH_HEIGHT = 650;

	int LOAD_WIDTH = 50;
	int LOAD_HEIGHT = 13;

	int m_loadSpacing = 45;
	int m_loadX = 55;
	int m_loadY = 520;

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

	bool m_useAltBitmaps = false;
	std::wstring m_altBackground {};
	std::wstring m_altLoad {};
	std::wstring m_altLoadFilled {};

	int m_statusFontSize = 14;
	int m_transparency = RGB(255, 0, 0);
	int m_statusColor = RGB(255, 255, 255);

	std::wstring m_fontFace {};
	int m_statusFontWeight = FW_ULTRALIGHT;

	std::wstring m_message {};
	int m_progress = 0;
};

extern NSSplashScreen* g_SplashScreen;
extern "C" __declspec(dllexport) void SetSplashMessage(const char* msg, int progress, bool close = false);
