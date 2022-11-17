// SplashScreen.cpp: implementation of the CSplashScreen class.
//
//////////////////////////////////////////////////////////////////////

#include "resource1.h"
#include "splash.h"
#include <stdexcept>

// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <tchar.h>
#include <string>
#include <chrono>

NSSplashScreen* g_SplashScreen;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
// static CSplashScreen *gDlg;
NSSplashScreen* NSSplashScreen::m_pSplashWnd = NULL;
ATOM NSSplashScreen::m_szWindowClass = 0;
BOOL NSSplashScreen::m_useStderr = FALSE;

NSSplashScreen::NSSplashScreen()
{

	redrawRect.left = 0;
	redrawRect.top = 0;
	redrawRect.right = SPLASH_WIDTH;
	redrawRect.bottom = SPLASH_HEIGHT;

	statusRect.left = SPLASH_WIDTH / 2 - 200;
	statusRect.top = 550;
	statusRect.right = SPLASH_WIDTH / 2 + 200;
	statusRect.bottom = 600;

	m_pSplashWnd = this;

	m_instance = GetModuleHandle(NULL);

	LPCTSTR szTitle = TEXT("Northstar Launcher");
	LPCTSTR szWindowClassName = TEXT("Northstar Launcher");

	if (m_szWindowClass == 0)
	{
		BOOL result = RegisterClass(szWindowClassName);
		if (!result)
			throw std::runtime_error("Failed to create window class");
	}

	DWORD exStyle = 0;

	RECT parentRect;
	GetWindowRect(GetDesktopWindow(), &parentRect);
	HWND hwnd = GetDesktopWindow();

	int xPos = parentRect.left + (parentRect.right - parentRect.left) / 2 - (SPLASH_WIDTH / 2);
	int yPos = parentRect.top + (parentRect.bottom - parentRect.top) / 2 - (SPLASH_HEIGHT / 2);

	HMENU menu = NULL;
	m_hWnd = CreateWindowExW(
		exStyle,
		szWindowClassName,
		szTitle,
		WS_EX_LAYERED | WS_POPUP | WS_VISIBLE,
		xPos,
		yPos,
		SPLASH_WIDTH,
		SPLASH_HEIGHT,
		m_hParentWnd,
		menu,
		m_instance,
		this);
	SetWindowLong(m_hWnd, GWL_EXSTYLE, GetWindowLong(m_hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
	SetLayeredWindowAttributes(m_hWnd, RGB(255, 0, 0), 0, LWA_COLORKEY | LWA_ALPHA);

	if (m_hWnd == NULL)
	{
		throw std::runtime_error("Failed to create window");
	}

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

	SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

BOOL NSSplashScreen::RegisterClass(LPCTSTR szWindowClassName)
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
		throw std::runtime_error("Failed to create window class");
	}
	return TRUE;
}

void NSSplashScreen::HideSplashScreen()
{
	// Destroy the window, and update the mainframe.
	if (m_hWnd != NULL)
	{
		std::thread fadeOut(
			[m_hWnd = m_hWnd]()
			{
				for (int i = 0; i < 128; i++)
				{
					std::this_thread::sleep_for(std::chrono::microseconds(2));
					SetLayeredWindowAttributes(m_hWnd, RGB(255, 0, 0), 255 - i * 2, LWA_COLORKEY | LWA_ALPHA);
					DestroyWindow(m_hWnd);
					if (m_hWnd && IsWindow(m_hWnd))
						UpdateWindow(m_hWnd);
				}
			});
		fadeOut.detach();
	}
}

LRESULT CALLBACK NSSplashScreen::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_PAINT:
		break;

	case WM_NCDESTROY:
		delete m_pSplashWnd;
		m_pSplashWnd = NULL;
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
HFONT NSSplashScreen::CreatePointFont(int pointSize, HDC dc)
{
	HFONT font;

	LOGFONT logicalFont;
	memset(&logicalFont, 0, sizeof(LOGFONT));
	logicalFont.lfWeight = FW_ULTRALIGHT;
	logicalFont.lfClipPrecision = CLIP_EMBEDDED;
	logicalFont.lfQuality = PROOF_QUALITY;
	logicalFont.lfOutPrecision = OUT_STRING_PRECIS;

	logicalFont.lfHeight = -MulDiv(pointSize, GetDeviceCaps(dc, LOGPIXELSY), 72); // pointSize * 10;
	font = CreateFontIndirect(&logicalFont);
	return font;
}

void NSSplashScreen::SetSplashMessage(const char* message, int progress, bool close)
{
	if (close)
	{
		HideSplashScreen();
		return;
	}
	m_progress = progress;
	std::string temp {message};
	m_message = std::wstring(temp.begin(), temp.end());
	InvalidateRect(m_hWnd, &redrawRect, false);
	Paint();
}

void NSSplashScreen::Paint()
{
	PAINTSTRUCT ps;
	HDC paintDC = BeginPaint(m_hWnd, &ps);

	HDC dc_splash = CreateCompatibleDC(paintDC);
	HDC dc_load = CreateCompatibleDC(paintDC);
	HDC dc_load_filled = CreateCompatibleDC(paintDC);

	// For some godawful reason copying a bitmap between two devices clears the source
	// Im not even fucking kidding
	// https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-bitblt#remarks
	m_bitmap = LoadBitmap(m_instance, MAKEINTRESOURCE(IDB_SPLASH));
	m_loadbar = LoadBitmap(m_instance, MAKEINTRESOURCE(IDB_LOAD));
	m_loadbar_filled = LoadBitmap(m_instance, MAKEINTRESOURCE(IDB_LOAD_FILLED));

	SelectObject(dc_splash, m_bitmap);
	SelectObject(dc_load, m_loadbar);
	SelectObject(dc_load_filled, m_loadbar_filled);

	SetStretchBltMode(paintDC, COLORONCOLOR);

	BitBlt(paintDC, 0, 0, SPLASH_WIDTH, SPLASH_HEIGHT, dc_splash, 0, 0,  SRCCOPY);
	for (int i = 0; i < 9; i++)
	{
		if (i >= m_progress)
			TransparentBlt(paintDC, 55 + i * 45, 520, 50, 13, dc_load, 0, 0, 50, 13, RGB(255, 0, 0));
		else
			TransparentBlt(paintDC, 55 + i * 45, 520, 50, 13, dc_load_filled, 0, 0, 50, 13, RGB(255, 0, 0));
	}

	HFONT font;
	HFONT productNameFont = CreatePointFont(14, paintDC);
	HFONT originalFont = (HFONT)SelectObject(paintDC, productNameFont);

	SetTextColor(paintDC, RGB(255, 255, 255));
	SetBkMode(paintDC, TRANSPARENT);

	DrawText(paintDC, m_message.c_str(), m_message.length(), &statusRect, DT_CENTER | DT_SINGLELINE);

	SelectObject(paintDC, originalFont);
	DeleteObject(productNameFont);

	EndPaint(m_hWnd, &ps);
}

// Used to expose splash screen to launcher dll
extern "C" __declspec(dllexport) void SetSplashMessage(const char* msg, int progress, bool close)
{
	if (g_SplashScreen)
		g_SplashScreen->SetSplashMessage(msg, progress, close);
}
