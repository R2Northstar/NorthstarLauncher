/*
Custom Splash Screen format:

{
	"img_background": "path_to_background.bmp",
	"img_load": "path_to_load.bmp",
	"img_load_filled": "path_to_load_filled.bmp",
	"load_spacing": 45,
	"load_pos": [200, 520],
	"status_rect": [0, 0, 0, 0],
	"status_fontsize": 14,
	"status_font": "",
	"status_weight": 400,
	"status_color": [0, 0, 0],
	"transparency": [255, 0, 0],
}

Use with parameter: `-alternateSplash=path_to_folder`
Path syntax is the same as for profiles
Images are loaded from the same directory as the splash json
Transparency can be calculated with the `RGB` macro
If `status_right` or `status_bottom` are 0, they will be set to the image width and height respectivel
`img_load` and `img_load_filled` must be the same size to work correctly
`load_pos` maps its members in order to LOAD_X and LOAD_Y
`status_rect` maps its members in order to LEFT, RIGHT, TOP, and BOTTOM
*/

#include "resource.h"

#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>

#define RAPIDJSON_NOMEMBERITERATORCLASS // need this for rapidjson
#define RAPIDJSON_HAS_STDSTRING 1

#include "rapidjson/error/en.h"
#include "rapidjson/document.h"
#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/writer.h"
typedef rapidjson::GenericDocument<rapidjson::UTF8<>> rapidjson_document;
namespace fs = std::filesystem;

#include "splash.h"
#include "dllmain.h"

NSSplashScreen* g_SplashScreen;

NSSplashScreen* NSSplashScreen::m_pSplashWnd = NULL;
ATOM NSSplashScreen::m_szWindowClass = 0;
BOOL NSSplashScreen::m_useStderr = FALSE;

void NSSplashScreen::LoadDefaults()
{
	redrawRect.left = 0;
	redrawRect.top = 0;
	redrawRect.right = SPLASH_WIDTH;
	redrawRect.bottom = SPLASH_HEIGHT;

	statusRect.left = SPLASH_WIDTH / 2 - 200;
	statusRect.top = 550;
	statusRect.right = SPLASH_WIDTH / 2 + 200;
	statusRect.bottom = 600;

	m_useAltBitmaps = false;
}

void NSSplashScreen::LoadFromFile(std::string& path)
{

	std::ifstream altSplashFile(path + "\\splash.json");
	std::stringstream altSplashStream;

	rapidjson_document doc;

	if (!altSplashFile.fail())
	{
		altSplashStream << altSplashFile.rdbuf();

		altSplashFile.close();
		doc.Parse<rapidjson::ParseFlag::kParseCommentsFlag | rapidjson::ParseFlag::kParseTrailingCommasFlag>(altSplashStream.str().c_str());
		if (!doc.IsObject())
		{
			throw std::runtime_error("Could not load altSplashScreen json");
			exit(-1);
		}
	}
	if (!doc.IsObject())
	{
		std::cout << "Invalid Alternate Splash Screen location." << std::endl;
		return;
	}
	auto test = doc.HasMember("img_background");

	HDC imageDC = ::CreateCompatibleDC(NULL);
	HBITMAP temp_bg = NULL;
	HBITMAP temp_load = NULL;
	if (doc.HasMember("img_background"))
	{
		std::string img_str = path + "\\" + doc["img_background"].GetString();
		m_altBackground = std::wstring(img_str.begin(), img_str.end());
		temp_bg = (HBITMAP)LoadImage(NULL, m_altBackground.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	}
	else
		temp_bg = LoadBitmap(m_instance, MAKEINTRESOURCE(IDB_SPLASH));

	if (doc.HasMember("img_load"))
	{
		std::string img_str = path + "\\" + doc["img_load"].GetString();
		m_altLoad = std::wstring(img_str.begin(), img_str.end());
		temp_load = (HBITMAP)LoadImage(NULL, m_altLoad.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
	}
	else
		temp_load = LoadBitmap(m_instance, MAKEINTRESOURCE(IDB_LOAD));

	if (doc.HasMember("img_load_filled"))
	{
		std::string img_str = path + "\\" + doc["img_load_filled"].GetString();
		m_altLoadFilled = std::wstring(img_str.begin(), img_str.end());
	}

	BITMAPINFO bitmapInfo;
	memset(&bitmapInfo, 0, sizeof(BITMAPINFOHEADER));
	bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	int scanLines = GetDIBits(
		imageDC, // handle to DC
		temp_bg, // handle to bitmap
		0, // first scan line to set
		0, // number of scan lines to copy
		NULL, // array for bitmap bits
		&bitmapInfo, // bitmap data buffer
		DIB_RGB_COLORS); // RGB or palette index

	SPLASH_WIDTH = bitmapInfo.bmiHeader.biWidth;
	SPLASH_HEIGHT = bitmapInfo.bmiHeader.biHeight;

	memset(&bitmapInfo, 0, sizeof(BITMAPINFOHEADER));
	bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	scanLines = GetDIBits(
		imageDC, // handle to DC
		temp_load, // handle to bitmap
		0, // first scan line to set
		0, // number of scan lines to copy
		NULL, // array for bitmap bits
		&bitmapInfo, // bitmap data buffer
		DIB_RGB_COLORS); // RGB or palette index

	LOAD_WIDTH = bitmapInfo.bmiHeader.biWidth;
	LOAD_HEIGHT = bitmapInfo.bmiHeader.biHeight;

	redrawRect.left = 0;
	redrawRect.top = 0;
	redrawRect.right = SPLASH_WIDTH;
	redrawRect.bottom = SPLASH_HEIGHT;

	if (doc.HasMember("status_rect") && doc["status_rect"].IsArray())
	{
		statusRect.left = doc["status_rect"][0].GetInt();
		statusRect.right = doc["status_rect"][1].GetInt();
		statusRect.top = doc["status_rect"][2].GetInt();
		statusRect.bottom = doc["status_rect"][3].GetInt();

		if (statusRect.right == 0)
			statusRect.right = SPLASH_WIDTH;
		if (statusRect.bottom == 0)
			statusRect.bottom = SPLASH_HEIGHT;
	}
	else
	{
		statusRect.left = 0;
		statusRect.top = 0;
		statusRect.right = SPLASH_WIDTH;
		statusRect.bottom = SPLASH_HEIGHT;
	}

	if (doc.HasMember("status_fontsize"))
		m_statusFontSize = doc["status_fontsize"].GetInt();

	if (doc.HasMember("status_font"))
	{
		std::string fontStr = doc["status_font"].GetString();
		m_fontFace = std::wstring(fontStr.begin(), fontStr.end());
	}

	if (doc.HasMember("status_weight"))
		m_statusFontWeight = doc["status_weight"].GetInt();

	if (doc.HasMember("status_color") && doc["status_color"].IsArray())
		m_statusColor = RGB(doc["status_color"][0].GetInt(), doc["status_color"][1].GetInt(), doc["status_color"][2].GetInt());

	if (doc.HasMember("transparency") && doc["transparency"].IsArray())
		m_transparency = RGB(doc["transparency"][0].GetInt(), doc["transparency"][1].GetInt(), doc["transparency"][2].GetInt());

	if (doc.HasMember("load_pos") && doc["load_pos"].IsArray())
	{
		m_loadX = doc["load_pos"][0].GetInt();
		m_loadY = doc["load_pos"][1].GetInt();
	}

	if (doc.HasMember("load_spacing"))
		m_loadSpacing = doc["load_spacing"].GetInt();

	m_useAltBitmaps = true;

	DeleteObject(temp_bg);
	DeleteObject(temp_load);
}

NSSplashScreen::NSSplashScreen(std::string altSplashPath)
{

	m_pSplashWnd = this;

	m_instance = handle;

	if (altSplashPath != "")
		LoadFromFile(altSplashPath);
	else
		LoadDefaults();

	LPCTSTR szTitle = TEXT("Northstar Launcher");
	LPCTSTR szWindowClassName = TEXT("Northstar Launcher");

	if (m_szWindowClass == 0)
	{
		BOOL result = RegisterClass(szWindowClassName);
		if (!result)
			throw std::runtime_error("Failed to create window class");
	}

	DWORD exStyle = WS_EX_LAYERED;

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
		WS_POPUP | WS_VISIBLE,
		xPos,
		yPos,
		SPLASH_WIDTH,
		SPLASH_HEIGHT,
		m_hParentWnd,
		menu,
		m_instance,
		this);
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

	SetWindowPos(m_hWnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

BOOL NSSplashScreen::RegisterClass(LPCTSTR szWindowClassName)
{
	m_instance = handle;
	// register class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = m_instance;
	wcex.hIcon = (HICON)LoadImage(
		m_instance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, ::GetSystemMetrics(IDI_ICON1), ::GetSystemMetrics(IDI_ICON1), 0);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = szWindowClassName;
	wcex.hIconSm = (HICON)LoadImage(
		m_instance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), 0);
	;
	m_szWindowClass = RegisterClassEx(&wcex);
	if (m_szWindowClass == 0)
	{
		throw std::runtime_error("Failed to create window class");
	}
	return TRUE;
}

void NSSplashScreen::HideSplashScreen()
{
	if (m_hWnd != NULL)
	{
		PostMessage(m_hWnd, WM_CLOSE, 0, 0);
		m_hWnd = NULL;
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
	int height = -MulDiv(pointSize, GetDeviceCaps(dc, LOGPIXELSY), 72); // pointSize * 10;

	return CreateFontW(
		height,
		0,
		0,
		0,
		m_statusFontWeight,
		FALSE,
		FALSE,
		FALSE,
		DEFAULT_CHARSET,
		OUT_OUTLINE_PRECIS,
		CLIP_EMBEDDED,
		PROOF_QUALITY,
		DEFAULT_PITCH,
		m_fontFace.c_str());
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
	if (m_useAltBitmaps)
	{
		// Alternate background must be the same size as the normal one
		// I don't care enough to support multiple sizes
		if (m_altBackground.length() != 0)
			m_bitmap = (HBITMAP)LoadImage(NULL, m_altBackground.c_str(), IMAGE_BITMAP, SPLASH_WIDTH, SPLASH_HEIGHT, LR_LOADFROMFILE);
		else
			m_bitmap = LoadBitmap(m_instance, MAKEINTRESOURCE(IDB_SPLASH));

		if (m_altLoad.length() != 0)
			m_loadbar = (HBITMAP)LoadImage(NULL, m_altLoad.c_str(), IMAGE_BITMAP, LOAD_WIDTH, LOAD_HEIGHT, LR_LOADFROMFILE);
		else
			m_loadbar = LoadBitmap(m_instance, MAKEINTRESOURCE(IDB_LOAD));

		if (m_altLoadFilled.length() != 0)
			m_loadbar_filled = (HBITMAP)LoadImage(NULL, m_altLoadFilled.c_str(), IMAGE_BITMAP, LOAD_WIDTH, LOAD_HEIGHT, LR_LOADFROMFILE);
		else
			m_loadbar_filled = LoadBitmap(m_instance, MAKEINTRESOURCE(IDB_LOAD_FILLED));
	}
	else
	{
		m_bitmap = LoadBitmap(m_instance, MAKEINTRESOURCE(IDB_SPLASH));
		m_loadbar = LoadBitmap(m_instance, MAKEINTRESOURCE(IDB_LOAD));
		m_loadbar_filled = LoadBitmap(m_instance, MAKEINTRESOURCE(IDB_LOAD_FILLED));
	}

	SelectObject(dc_splash, m_bitmap);
	SelectObject(dc_load, m_loadbar);
	SelectObject(dc_load_filled, m_loadbar_filled);

	int test = RGB(255, 0, 0);

	SetStretchBltMode(paintDC, COLORONCOLOR);

	BitBlt(paintDC, 0, 0, SPLASH_WIDTH, SPLASH_HEIGHT, dc_splash, 0, 0, SRCCOPY);
	for (int i = 0; i < 9; i++)
	{
		if (i >= m_progress)
			TransparentBlt(
				paintDC,
				m_loadX + i * m_loadSpacing,
				m_loadY,
				LOAD_WIDTH,
				LOAD_HEIGHT,
				dc_load,
				0,
				0,
				LOAD_WIDTH,
				LOAD_HEIGHT,
				RGB(255, 0, 0));
		else
			TransparentBlt(
				paintDC,
				m_loadX + i * m_loadSpacing,
				m_loadY,
				LOAD_WIDTH,
				LOAD_HEIGHT,
				dc_load_filled,
				0,
				0,
				LOAD_WIDTH,
				LOAD_HEIGHT,
				RGB(255, 0, 0));
	}

	HFONT statusFont = CreatePointFont(m_statusFontSize, paintDC);
	SelectObject(paintDC, statusFont);

	SetTextColor(paintDC, m_statusColor);
	SetBkMode(paintDC, TRANSPARENT);

	DrawText(paintDC, m_message.c_str(), static_cast<int>(m_message.length()), &statusRect, DT_CENTER | DT_SINGLELINE);

	SelectObject(paintDC, statusFont);
	DeleteObject(statusFont);
	DeleteObject(dc_splash);
	DeleteObject(dc_load);
	DeleteObject(dc_load_filled);
	DeleteObject(m_bitmap);
	DeleteObject(m_loadbar);
	DeleteObject(m_loadbar_filled);

	EndPaint(m_hWnd, &ps);
}

extern "C" __declspec(dllexport) void InitializeSplashScreen(const char* loadPath)
{
	g_SplashScreen = new NSSplashScreen(loadPath);
}


// Used to expose splash screen to launcher dll
extern "C" __declspec(dllexport) void SetSplashMessage(const char* msg, int progress, bool close)
{
	if (g_SplashScreen)
		g_SplashScreen->SetSplashMessage(msg, progress, close);
}
