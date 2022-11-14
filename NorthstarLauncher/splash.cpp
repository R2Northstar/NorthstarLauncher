// SplashScreen.cpp: implementation of the CSplashScreen class.
//
//////////////////////////////////////////////////////////////////////

#include "resource1.h"
#include "splash.h"
#include <crtdbg.h> // for _ASSERT()
#include <stdio.h> // for vsprintf
#include <stdarg.h> // for vsprintf
#include <stdexcept>

// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <string>
#include <chrono>

CSplashScreen* g_SplashScreen;

int SRC_X = 512;
int SRC_Y = 650;
int DST_X = 512;
int DST_Y = 650;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
// static CSplashScreen *gDlg;
CSplashScreen* CSplashScreen::m_pSplashWnd = NULL;
ATOM CSplashScreen::m_szWindowClass = 0;
BOOL CSplashScreen::m_useStderr = FALSE;

CSplashScreen::CSplashScreen(HWND parentWnd)
{

	redrawRect.left = 0;
	redrawRect.top = 0;
	redrawRect.right = DST_X;
	redrawRect.bottom = DST_Y;

	statusRect.left = DST_X / 2 - 200;
	statusRect.top = 550;
	statusRect.right = DST_X / 2 + 200;
	statusRect.bottom = 600;

	m_hParentWnd = parentWnd;

	m_pSplashWnd = this;

	m_instance = GetModuleHandle(NULL);

	LPCTSTR szTitle = TEXT("");
	LPCTSTR szWindowClassName = TEXT("SplashScreen");

	// register splash window class if not already registered
	if (m_szWindowClass == 0)
	{
		BOOL result = RegisterClass(szWindowClassName);
		if (!result)
			throw std::runtime_error("Failed to create window class");
	}

	DWORD exStyle = 0;
	int xPos = 0;
	int yPos = 0;
	int width = DST_X;
	int height = DST_Y;

	// if parent window, center it on the parent window. otherwise center it on the screen
	RECT parentRect;
	if (m_hParentWnd == NULL)
	{
		::GetWindowRect(GetDesktopWindow(), &parentRect);
	}
	else
	{
		::GetWindowRect(m_hParentWnd, &parentRect);
	}
	HWND hwnd = GetDesktopWindow();

	xPos = parentRect.left + (parentRect.right - parentRect.left) / 2 - (width / 2);
	yPos = parentRect.top + (parentRect.bottom - parentRect.top) / 2 - (height / 2);

	HMENU menu = NULL;
	m_hWnd = CreateWindowEx(
		exStyle,
		szWindowClassName,
		szTitle,
		WS_EX_LAYERED | WS_POPUP | WS_VISIBLE,
		xPos,
		yPos,
		width,
		height,
		m_hParentWnd,
		menu,
		m_instance,
		this);

	SetWindowLong(m_hWnd, GWL_EXSTYLE, GetWindowLong(m_hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
	SetLayeredWindowAttributes(m_hWnd, RGB(255, 0, 0), 0, LWA_COLORKEY | LWA_ALPHA);

	std::thread fadeIn(
		[m_hWnd = m_hWnd]()
		{
			for (int i = 0; i < 128; i++)
			{
				std::this_thread::sleep_for(std::chrono::microseconds(2));
				SetLayeredWindowAttributes(m_hWnd, RGB(255, 0, 0), i * 2, LWA_COLORKEY | LWA_ALPHA);
			}
		});
	fadeIn.detach();

	if (m_hWnd == NULL)
	{
		throw std::runtime_error("Failed to create window");
	}

	// if no parent window, make it a topmost, so eventual application window will appear under it
	if (m_hParentWnd == NULL)
	{
		::SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
}

void CSplashScreen::HideSplashScreen()
{

	// Destroy the window, and update the mainframe.
	if (m_pSplashWnd != NULL)
	{
		HWND hParentWnd = m_pSplashWnd->m_hParentWnd;
		::DestroyWindow(m_pSplashWnd->m_hWnd);
		if (hParentWnd && ::IsWindow(hParentWnd))
			::UpdateWindow(hParentWnd);
	}
}

void CSplashScreen::ReportError(LPCTSTR format, ...)
{
	TCHAR buffer[4096];
	va_list argp;
	va_start(argp, format);
	_tcprintf(buffer, format, argp);
	va_end(argp);
	MessageBox(m_hWnd, buffer, TEXT("Error"), MB_ICONERROR);
}
BOOL CSplashScreen::RegisterClass(LPCTSTR szWindowClassName)
{
	m_instance = GetModuleHandle(NULL);

	// register class
	DWORD lastError;
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = m_instance;
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClassName;
	wcex.hIconSm = NULL;

	m_szWindowClass = RegisterClassEx(&wcex);
	if (m_szWindowClass == 0)
	{
		ReportError(TEXT("Failed to register class"));
		return FALSE;
	}
	return TRUE;
}

LRESULT CALLBACK CSplashScreen::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{

	case WM_PAINT:
		// m_pSplashWnd->Paint ();
		break;

	case WM_NCDESTROY:
		delete m_pSplashWnd;
		m_pSplashWnd = NULL;
		break;

	case WM_TIMER:
		m_pSplashWnd->HideSplashScreen();
		break;

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_NCLBUTTONDOWN:
	case WM_NCRBUTTONDOWN:
	case WM_NCMBUTTONDOWN:
		m_pSplashWnd->HideSplashScreen();
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
HFONT CSplashScreen::CreatePointFont(int pointSize, HDC dc)
{
	HFONT font;

	LOGFONT logicalFont;
	memset(&logicalFont, 0, sizeof(LOGFONT));
	logicalFont.lfWeight = FW_ULTRALIGHT;
	logicalFont.lfClipPrecision = CLIP_EMBEDDED;
	logicalFont.lfQuality = PROOF_QUALITY;
	logicalFont.lfOutPrecision = OUT_STRING_PRECIS;
	logicalFont.lfPitchAndFamily = FF_DONTCARE;

	logicalFont.lfHeight = -MulDiv(pointSize, GetDeviceCaps(dc, LOGPIXELSY), 72); // pointSize * 10;
	font = CreateFontIndirect(&logicalFont);
	return font;
}

void CSplashScreen::SetSplashMessage(const char* message, int progress, bool close)
{
	if (close)
	{
		std::thread fadeOut(
			[m_hWnd = m_hWnd]()
			{
				for (int i = 0; i < 128; i++)
				{
					std::this_thread::sleep_for(std::chrono::microseconds(2));
					SetLayeredWindowAttributes(m_hWnd, RGB(255, 0, 0), 255 - i * 2, LWA_COLORKEY | LWA_ALPHA);
				}
			});
		fadeOut.detach();
		::ShowWindow(m_hWnd, SW_HIDE);
		::UpdateWindow(m_hWnd);
		::DestroyWindow(m_hWnd);
		return;
	}
	m_progress = progress;
	std::string temp {message};
	m_message = std::wstring(temp.begin(), temp.end());
	InvalidateRect(m_hWnd, &redrawRect, false);
	Paint();
}

void CSplashScreen::Paint()
{
	PAINTSTRUCT ps;
	HDC paintDC = BeginPaint(m_hWnd, &ps);

	HDC dc_splash = ::CreateCompatibleDC(paintDC);
	HDC dc_load = ::CreateCompatibleDC(paintDC);
	HDC dc_load_filled = ::CreateCompatibleDC(paintDC);

	// For some godawful reason copying a bitmap between two devices clears the source
	// Im not even fucking kidding
	// https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-bitblt#remarks
	m_bitmap = LoadBitmap(m_instance, MAKEINTRESOURCE(IDB_SPLASH));
	m_pip = LoadBitmap(m_instance, MAKEINTRESOURCE(IDB_LOAD));
	m_pipfull = LoadBitmap(m_instance, MAKEINTRESOURCE(IDB_LOAD_FILLED));

	SelectObject(dc_splash, m_bitmap);
	SelectObject(dc_load, m_pip);
	SelectObject(dc_load_filled, m_pipfull);

	SetStretchBltMode(paintDC, COLORONCOLOR);

	StretchBlt(paintDC, 0, 0, DST_X, DST_Y, dc_splash, 0, 0, SRC_X, SRC_Y, SRCCOPY);
	for (int i = 0; i < 9; i++)
	{
		if (i >= m_progress)
			TransparentBlt(paintDC, 55 + i * 45, 520, 50, 13, dc_load, 0, 0, 50, 13, RGB(255, 0, 0));
		else
			TransparentBlt(paintDC, 55 + i * 45, 520, 50, 13, dc_load_filled, 0, 0, 50, 13, RGB(255, 0, 0));
	}

	HFONT font;
	HFONT productNameFont = CreatePointFont(22, paintDC);
	HFONT originalFont = (HFONT)SelectObject(paintDC, productNameFont);

	SetTextColor(paintDC, RGB(255, 255, 255));
	SetBkMode(paintDC, TRANSPARENT);

	DrawText(paintDC, m_message.c_str(), m_message.length(), &statusRect, DT_CENTER | DT_SINGLELINE);

	SelectObject(paintDC, originalFont);
	DeleteObject(productNameFont);

	EndPaint(m_hWnd, &ps);
}

extern "C" __declspec(dllexport) void SetSplashMessage(const char* msg, int progress, bool close)
{
	if (g_SplashScreen)
		g_SplashScreen->SetSplashMessage(msg, progress, close);
}
