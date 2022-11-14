#pragma once
// SplashScreen.h: interface for the CSplashScreen class.
//
//////////////////////////////////////////////////////////////////////

// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <string>

class CSplashScreen
{
  public:
	CSplashScreen(HWND parentWnd);

	static void HideSplashScreen();

	void SetSplashMessage(const char* message, int progress, bool close);

  protected:
	BOOL Create(HWND pParentWnd = NULL);
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	void ReportError(LPCTSTR format, ...);
	BOOL RegisterClass(LPCTSTR szWindowClassName);

	HFONT CreatePointFont(int pointSize, HDC dc);
	void Paint();

  private:
	RECT redrawRect;
	RECT statusRect;
	HWND m_hDlg;
	HWND m_hParentWnd;
	HWND m_hWnd;
	HINSTANCE m_instance;
	static CSplashScreen* m_pSplashWnd;
	static ATOM m_szWindowClass;
	static BOOL m_useStderr;

	HBITMAP m_bitmap;
	HBITMAP m_pip;
	HBITMAP m_pipfull;

	std::wstring m_message;
	int m_progress;
};

extern CSplashScreen* g_SplashScreen;
extern "C" __declspec(dllexport) void SetSplashMessage(const char* msg, int progress, bool close = false);
