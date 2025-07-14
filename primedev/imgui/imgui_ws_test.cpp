#include "imgui_ws_test.h"
#include "imgui-ws/imgui-ws.h"
#include "imgui-ws/imgui-draw-data-compressor.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_dx11.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "core/vanilla.h"
#include "core/tier1.h"
#include "engine/r2engine.h"
#include <mutex>

#define CINTERFACE
#include "dxgi.h"
#undef CINTERFACE
#include "d3d11.h"

// thread-local to allow for different threads to have different contexts,
// i.e. render thread for most menus, other threads for squirrel things
thread_local ImGuiContext* ImGuiThreadContext;

static ImGuiContext* ImGuiRenderThreadContext;
static ImGuiContext* ImGuiSquirrelThreadContext;

static ImGuiWS imguiWS;
static bool isInited = false;

static ID3D11Device** device = nullptr; 
static ID3D11DeviceContext** deviceContext = nullptr;
static IDXGISwapChain** swapChain = nullptr;

static std::mutex sqRenderSnapshot_mutex;
static ImDrawDataSnapshot sqRenderSnapshot;

static ConVar* Cvar_imgui_mode = nullptr;
static ConVar* Cvar_imgui_ws_port = nullptr;

enum CursorCode
{
	dc_user,
	dc_none,
	dc_arrow,
	dc_ibeam,
	dc_hourglass,
	dc_waitarrow,
	dc_crosshair,
	dc_up,
	dc_sizenwse,
	dc_sizenesw,
	dc_sizewe,
	dc_sizens,
	dc_sizeall,
	dc_no,
	dc_hand,
	dc_blank,
	dc_last,
};

static bool engineCursorVisible = false;

class ISurface
{
public:
	struct VTable
	{
		void* unknown1[61];
		void (*SetCursor)(ISurface* surface, unsigned int cursor);
		bool (*IsCursorVisible)(ISurface* surface);
		void* unknown2[11];
		void (*UnlockCursor)(ISurface* surface);
		void (*LockCursor)(ISurface* surface);
	};

	VTable* m_vtable;
};

static ISurface* VGUI_Surface031 = nullptr;

static int GetImGuiMode()
{
	if (!Cvar_imgui_mode)
		return 0;

	// vanilla compat mode - don't allow drawing on the screen because I don't want people to get accidentally fairfight banned
	// they can still access the websocket version though
	if (g_pVanillaCompatibility->GetVanillaCompatibility())
		return 0;

	return Cvar_imgui_mode->GetInt();
}

static ImGuiKey toImGuiKey(int32_t keyCode)
{
	switch (keyCode)
	{
	case 8:
		return ImGuiKey_Backspace;
	case 9:
		return ImGuiKey_Tab;
	case 13:
		return ImGuiKey_Enter;
	case 16:
		return ImGuiKey_ModShift;
	case 17:
		return ImGuiKey_ModCtrl;
	case 18:
		return ImGuiKey_ModAlt;
	case 19:
		return ImGuiKey_Pause;
	case 20:
		return ImGuiKey_CapsLock;
	case 27:
		return ImGuiKey_Escape;
	case 32:
		return ImGuiKey_Space;
	case 33:
		return ImGuiKey_PageUp;
	case 34:
		return ImGuiKey_PageDown;
	case 35:
		return ImGuiKey_End;
	case 36:
		return ImGuiKey_Home;
	case 37:
		return ImGuiKey_LeftArrow;
	case 38:
		return ImGuiKey_UpArrow;
	case 39:
		return ImGuiKey_RightArrow;
	case 40:
		return ImGuiKey_DownArrow;
	case 45:
		return ImGuiKey_Insert;
	case 46:
		return ImGuiKey_Delete;
	case 48:
		return ImGuiKey_0;
	case 49:
		return ImGuiKey_1;
	case 50:
		return ImGuiKey_2;
	case 51:
		return ImGuiKey_3;
	case 52:
		return ImGuiKey_4;
	case 53:
		return ImGuiKey_5;
	case 54:
		return ImGuiKey_6;
	case 55:
		return ImGuiKey_7;
	case 56:
		return ImGuiKey_8;
	case 57:
		return ImGuiKey_9;
	case 65:
		return ImGuiKey_A;
	case 66:
		return ImGuiKey_B;
	case 67:
		return ImGuiKey_C;
	case 68:
		return ImGuiKey_D;
	case 69:
		return ImGuiKey_E;
	case 70:
		return ImGuiKey_F;
	case 71:
		return ImGuiKey_G;
	case 72:
		return ImGuiKey_H;
	case 73:
		return ImGuiKey_I;
	case 74:
		return ImGuiKey_J;
	case 75:
		return ImGuiKey_K;
	case 76:
		return ImGuiKey_L;
	case 77:
		return ImGuiKey_M;
	case 78:
		return ImGuiKey_N;
	case 79:
		return ImGuiKey_O;
	case 80:
		return ImGuiKey_P;
	case 81:
		return ImGuiKey_Q;
	case 82:
		return ImGuiKey_R;
	case 83:
		return ImGuiKey_S;
	case 84:
		return ImGuiKey_T;
	case 85:
		return ImGuiKey_U;
	case 86:
		return ImGuiKey_V;
	case 87:
		return ImGuiKey_W;
	case 88:
		return ImGuiKey_X;
	case 89:
		return ImGuiKey_Y;
	case 90:
		return ImGuiKey_Z;

	// case 91: return ImGuiKey_LWin;
	// case 92: return ImGuiKey_RWin;
	// case 93: return ImGuiKey_Apps;
	case 91:
		return ImGuiKey_ModSuper;
	case 92:
		return ImGuiKey_ModSuper;
	case 93:
		return ImGuiKey_ModSuper;

	case 96:
		return ImGuiKey_Keypad0;
	case 97:
		return ImGuiKey_Keypad1;
	case 98:
		return ImGuiKey_Keypad2;
	case 99:
		return ImGuiKey_Keypad3;
	case 100:
		return ImGuiKey_Keypad4;
	case 101:
		return ImGuiKey_Keypad5;
	case 102:
		return ImGuiKey_Keypad6;
	case 103:
		return ImGuiKey_Keypad7;
	case 104:
		return ImGuiKey_Keypad8;
	case 105:
		return ImGuiKey_Keypad9;
	case 106:
		return ImGuiKey_KeypadMultiply;
	case 107:
		return ImGuiKey_KeypadAdd;
	case 108:
		return ImGuiKey_KeypadEnter;
	case 109:
		return ImGuiKey_KeypadSubtract;
	case 110:
		return ImGuiKey_KeypadDecimal;
	case 111:
		return ImGuiKey_KeypadDivide;
	case 112:
		return ImGuiKey_F1;
	case 113:
		return ImGuiKey_F2;
	case 114:
		return ImGuiKey_F3;
	case 115:
		return ImGuiKey_F4;
	case 116:
		return ImGuiKey_F5;
	case 117:
		return ImGuiKey_F6;
	case 118:
		return ImGuiKey_F7;
	case 119:
		return ImGuiKey_F8;
	case 120:
		return ImGuiKey_F9;
	case 121:
		return ImGuiKey_F10;
	case 122:
		return ImGuiKey_F11;
	case 123:
		return ImGuiKey_F12;
	case 144:
		return ImGuiKey_NumLock;
	case 145:
		return ImGuiKey_ScrollLock;
	case 186:
		return ImGuiKey_Semicolon;
	case 187:
		return ImGuiKey_Equal;
	case 188:
		return ImGuiKey_Comma;
	case 189:
		return ImGuiKey_Minus;
	case 190:
		return ImGuiKey_Period;
	case 191:
		return ImGuiKey_Slash;
	case 219:
		return ImGuiKey_LeftBracket;
	case 220:
		return ImGuiKey_Backslash;
	case 221:
		return ImGuiKey_RightBracket;
	case 222:
		return ImGuiKey_Apostrophe;
	default:
		return ImGuiKey_COUNT;
	}
}

struct WsState
{
	WsState() {}
	struct InputEvent
	{
		enum Type
		{
			EKey,
			EMousePos,
			EMouseButton,
			EMouseWheel,
		};

		Type type;

		bool isDown = false;

		ImGuiKey key = ImGuiKey_COUNT;
		ImGuiMouseButton mouseButton = -1;
		ImVec2 mousePos;
		float mouseWheelX = 0.0f;
		float mouseWheelY = 0.0f;
	};

	int port = 5000;

	// client input
	std::vector<InputEvent> inputEvents;
	std::string lastAddText = "";

	void handle(ImGuiWS::Event&& event);
	void update();
};

static WsState state;

void WsState::handle(ImGuiWS::Event&& event)
{
	ImGuiMouseButton butImGui = event.mouse_but;
	// map the JS button code to Dear ImGui's button code
	switch (event.mouse_but)
	{
	case 1:
		butImGui = ImGuiMouseButton_Middle;
		break;
	case 2:
		butImGui = ImGuiMouseButton_Right;
		break;
	}

	switch (event.type)
	{
	case ImGuiWS::Event::Connected:
	case ImGuiWS::Event::Disconnected:
		break;
	case ImGuiWS::Event::MouseMove:
		inputEvents.push_back({InputEvent::Type::EMousePos, false, ImGuiKey_COUNT, -1, {event.mouse_x, event.mouse_y}, 0.0f, 0.0f});
		break;
	case ImGuiWS::Event::MouseDown:
	case ImGuiWS::Event::MouseUp:
		inputEvents.push_back({InputEvent::Type::EMouseButton, event.type == ImGuiWS::Event::MouseDown, ImGuiKey_COUNT, butImGui, {event.mouse_x, event.mouse_y}, 0.0f, 0.0f});
		break;
	case ImGuiWS::Event::MouseWheel:
		inputEvents.push_back({InputEvent::Type::EMouseWheel, false, ImGuiKey_COUNT, -1, {}, event.wheel_x, event.wheel_y});
		break;
	case ImGuiWS::Event::KeyUp:
	case ImGuiWS::Event::KeyDown:
		if (event.key > 0)
		{
			ImGuiKey keyImGui = toImGuiKey(event.key);
			inputEvents.push_back({InputEvent::Type::EKey, event.type == ImGuiWS::Event::KeyUp, keyImGui, -1, {}, 0.0f, 0.0f});
		}
		break;
	case ImGuiWS::Event::KeyPress:
		lastAddText.resize(1);
		lastAddText[0] = event.key;
		break;
	default:
		spdlog::warn("ImGui: Unknown input event\n");
	}
}

void WsState::update()
{
	auto& io = ImGui::GetIO();

	if (lastAddText.size() > 0)
	{
		io.AddInputCharactersUTF8(lastAddText.c_str());
	}

	for (const auto& event : inputEvents)
	{
		switch (event.type)
		{
		case InputEvent::Type::EKey:
		{
			io.AddKeyEvent(event.key, event.isDown);
		}
		break;
		case InputEvent::Type::EMousePos:
		{
			io.AddMousePosEvent(event.mousePos.x, event.mousePos.y);
		}
		break;
		case InputEvent::Type::EMouseButton:
		{
			io.AddMouseButtonEvent(event.mouseButton, event.isDown);
			io.AddMousePosEvent(event.mousePos.x, event.mousePos.y);
		}
		break;
		case InputEvent::Type::EMouseWheel:
		{
			io.AddMouseWheelEvent(event.mouseWheelX, event.mouseWheelY);
		}
		break;
		};
		inputEvents.clear();
		lastAddText = "";
	}
}

// a menu for debugging the ImGui integration systems
static void RenderImGuiDebug()
{
	ImGui::Text("Port: %i", state.port);
	ImGui::Text("Engine cursor visible: %i", engineCursorVisible);
}

// a small panel for controlling the imgui_mode
static void RenderImGuiDebugControls()
{
	if (ImGui::Begin("Debug Controls", 0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize))
	{
		if (GetImGuiMode() == 2)
		{
			ImGui::Text("ImGui debug is in overlay mode");
			ImGui::NewLine();
			ImGui::Text("Enter 'imgui_mode 0' into the console to close ImGui");
			ImGui::Text("Enter 'imgui_mode 1' into the console to return to normal mode");

			ImGui::End();
			return;
		}

		if (ImGui::Button("Close"))
			Cvar_imgui_mode->SetValue(0);
		if (ImGui::Button("Overlay Mode"))
			Cvar_imgui_mode->SetValue(2);
	}
	ImGui::End();
}

void RenderScriptThing(const char* message)
{
	// context was set up on the other thread, so the TLS variable isnt set
	if (ImGui::GetCurrentContext() == nullptr)
		ImGui::SetCurrentContext(ImGuiSquirrelThreadContext);
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();

	for (int i = 0; i < 10; ++i)
	{
		ImGui::NewFrame();
		ImGui::PushStyleColor(ImGuiCol_TitleBgActive, {0.226, 0.16, 0.476, 1});

		if (ImGui::Begin("HELLO WORLD", 0, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::TextColored({0.4, 1, 0.4, 1}, "This text is green!");
			ImGui::TextColored({1, 0.4, 0.4, 1}, "This text is red!");
			ImGui::Separator();
			ImGui::Text("The following text is from UI script:\n\n%s", message);
		}
		else
		{
			spdlog::error("NO IMGUI MENU????");
		}
		ImGui::End();

		ImGui::PopStyleColor(1);

		ImGui::Render();

		const std::lock_guard<std::mutex> lock(sqRenderSnapshot_mutex);
		sqRenderSnapshot.SnapUsingSwap(ImGui::GetDrawData(), ImGui::GetTime());
	}
}

void ImGuiDisplay::Start()
{
	state.port = Cvar_imgui_ws_port->GetInt();

	IMGUI_CHECKVERSION();
	m_context = ImGui::CreateContext();
	ImGuiRenderThreadContext = m_context;
	strcpy(ImGuiRenderThreadContext->ContextName, "Render");
	// ImGui::GetIO().MouseDrawCursor = true;

	// get the hwnd from directx
	DXGI_SWAP_CHAIN_DESC desc;
	(*swapChain)->lpVtbl->GetDesc(*swapChain, &desc);

	ImGui_ImplWin32_Init(desc.OutputWindow);
	ImGui_ImplDX11_Init(*device, *deviceContext);

	ImGui::StyleColorsDark();
	ImGui::GetStyle().AntiAliasedFill = false;
	ImGui::GetStyle().AntiAliasedLines = false;
	ImGui::GetStyle().WindowRounding = 0.0f;
	ImGui::GetStyle().ScrollbarRounding = 0.0f;

	// setup imgui-ws
	imguiWS.init(state.port, "./", {""});

	// prepare font texture
	{
		unsigned char* pixels;
		int width, height;
		ImGui::GetIO().Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);
		imguiWS.setTexture(0, ImGuiWS::Texture::Type::Alpha8, width, height, (const char*)pixels);
	}

	// todo: move this to UI vm instantiation?
	if (!ImGuiSquirrelThreadContext)
	{
		ImGuiSquirrelThreadContext = ImGui::CreateContext();
		strcpy(ImGuiSquirrelThreadContext->ContextName, "Squirrel");

		ImGui::SetCurrentContext(ImGuiSquirrelThreadContext);
		ImGui_ImplWin32_Init(desc.OutputWindow);
		ImGui_ImplDX11_Init(*device, *deviceContext);
		ImGui::SetCurrentContext(ImGuiRenderThreadContext);
	}

	isInited = true;
}

void ImGuiDisplay::Render()
{
	if (!isInited)
		Start();

	auto& io = ImGui::GetIO();

	// websocket event handling
	auto events = imguiWS.takeEvents();
	for (auto& event : events)
	{
		state.handle(std::move(event));
	}
	state.update();

	ImGui::SetCurrentContext(m_context);
	ImGui_ImplDX11_NewFrame();
	const ImVec2 lastSize = io.DisplaySize;
	ImGui_ImplWin32_NewFrame();
	// when you alt tab in fullscreen, the client size goes to 0 and
	// the websocket therefore scales to 0, so don't let that happen.
	if (io.DisplaySize.x == 0 || io.DisplaySize.y == 0)
		io.DisplaySize = lastSize;
	ImGui::NewFrame();

	const bool isOverlay = GetImGuiMode() == 2;
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, isOverlay ? 0.75f : 1.f);

	if (GetImGuiMode())
		RenderImGuiDebugControls();

	for (auto& menu : m_menus)
		menu.Render();

	// dear imgui demo. please never delete this.
	if (m_showDemoWindow)
		ImGui::ShowDemoWindow(&m_showDemoWindow);

	// main menu bar
	if (ImGui::BeginMainMenuBar())
	{
		ImGui::MenuItem("DearImGui Demo", 0, &m_showDemoWindow);
		for (auto& menu : m_menus)
			menu.RenderMenuItem();

		ImGui::EndMainMenuBar();
	}

	// clear our overlay style
	ImGui::PopStyleVar();

	// generate ImDrawData
	ImGui::Render();

	// store ImDrawData for asynchronous dispatching to WS clients
	auto* drawData = ImGui::GetDrawData();
	imguiWS.setDrawData(drawData);
	if (GetImGuiMode())
		ImGui_ImplDX11_RenderDrawData(drawData);
}

void ImGuiDisplay::Shutdown()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext(m_context);
	isInited = false;
	m_menus.clear();
}

void ImGuiDisplay::RegisterMenu(const char* name, ImGuiRenderCallback callback, const char* shortcut)
{
	m_menus.emplace_back(name, callback, shortcut);
}

ImGuiDisplay& ImGuiDisplay::GetInstance()
{
	static ImGuiDisplay* display = nullptr;

	if (display == nullptr)
	{
		display = new ImGuiDisplay();
		// add the ImGui debug menu always first
		display->RegisterMenu("ImGui Debug", RenderImGuiDebug, nullptr);
	}

	return *display;
}

static HRESULT (*o_pPresent)(IDXGISwapChain* This, UINT SyncInterval, UINT Flags) = nullptr;
static HRESULT h_Present(IDXGISwapChain* This, UINT SyncInterval, UINT Flags)
{
	{
		const std::lock_guard<std::mutex> lock(sqRenderSnapshot_mutex);
		if (&sqRenderSnapshot.DrawData)
		{
			ImGui_ImplDX11_RenderDrawData(&sqRenderSnapshot.DrawData); // todo: mutex for this in some way? bad things are happening
		}
	}

	// render our ImGui, todo: handle draw data on MainThread and only render it on RenderThread? I'm concerned about threading and fetching server/client data for ImGui rendering.
	ImGuiDisplay::GetInstance().Render();

	return o_pPresent(This, SyncInterval, Flags);
}

// this does various things, including initing the swapChain
static __int64 (*o_pSub_15470)(HWND hWnd, __int64 a2) = nullptr;
static __int64 h_sub_15470(HWND hWnd, __int64 a2)
{
	// swapChain is inited after here, so now we can hook the present func
	auto ret = o_pSub_15470(hWnd, a2);

	// create present hook
	auto* present = (*swapChain)->lpVtbl->Present;
	MH_CreateHook(present, h_Present, (void**)&o_pPresent);
	MH_EnableHook(present);

	return ret;
}

static bool IsKeyMsg(UINT uMsg)
{
	return uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST;
}

static bool IsMouseMsg(UINT uMsg)
{
	if (uMsg == WM_INPUT)
		return true;
	return uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandlerEx(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, ImGuiIO& io);
static int (*o_pGameWndProc)(void* game, const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = nullptr;
static int h_GameWndProc(void* game, const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//spdlog::warn("MESSAGE: {}", uMsg);

	auto& instance = ImGuiDisplay::GetInstance();

	// debug imgui, takes all mouse and keyboard input while not in overlay-mode
	if (ImGuiRenderThreadContext != nullptr && GetImGuiMode() == 1)
	{
		auto res = ImGui_ImplWin32_WndProcHandlerEx(hWnd, uMsg, wParam, lParam, ImGuiRenderThreadContext->IO);

		if (IsMouseMsg(uMsg) || IsKeyMsg(uMsg))
			return res;
	}

	// squirrel imgui, only takes input when it needs to.
	if (ImGuiSquirrelThreadContext != nullptr)
	{
		auto res = ImGui_ImplWin32_WndProcHandlerEx(hWnd, uMsg, wParam, lParam, ImGuiSquirrelThreadContext->IO);

		if (IsMouseMsg(uMsg) && ImGuiSquirrelThreadContext->IO.WantCaptureMouse)
			return res;
		if (IsKeyMsg(uMsg) && ImGuiSquirrelThreadContext->IO.WantCaptureKeyboard)
			return res;
	}

	//spdlog::warn("GOT THROUGH");

	return o_pGameWndProc(game, hWnd, uMsg, wParam, lParam);
}

static void (*o_pSetCursor)(ISurface* surface, unsigned int cursor) = nullptr;
static void h_setCursor(ISurface* surface, unsigned int cursor)
{
	// keep track of if the game wants a cursor
	engineCursorVisible = (cursor != dc_user && cursor != dc_none && cursor != dc_blank);

	o_pSetCursor(surface, cursor);
}

static void (*o_pLockCursor)(ISurface* surface) = nullptr;
static void h_lockCursor(ISurface* surface)
{
	// only allow locking the cursor if ImGui doesn't want it
	if (GetImGuiMode() == 1)
		return;

	o_pLockCursor(surface);
}

ON_DLL_LOAD("materialsystem_dx11.dll", ImGuiMaterialSystem, (CModule module))
{
	device = module.Offset(0x14E8DD0).RCast<ID3D11Device**>();
	deviceContext = module.Offset(0x14E8DD8).RCast<ID3D11DeviceContext**>();
	swapChain = module.Offset(0x14EE258).RCast<IDXGISwapChain**>();

	o_pSub_15470 = module.Offset(0x15470).RCast<decltype(o_pSub_15470)>();
	HookAttach(&(PVOID&)o_pSub_15470, h_sub_15470);

	Cvar_imgui_ws_port = new ConVar("imgui_ws_port", "5000", FCVAR_GAMEDLL | FCVAR_DONTRECORD, "The port that the ImGui debug display is hosted on");
}

ON_DLL_LOAD_RELIESON("inputsystem.dll", ImGuiInput, ConVar, (CModule module))
{
	o_pGameWndProc = module.Offset(0x8B80).RCast<decltype(o_pGameWndProc)>();
	HookAttach(&(PVOID&)o_pGameWndProc, h_GameWndProc);

	Cvar_imgui_mode = new ConVar("imgui_mode", "0", FCVAR_GAMEDLL | FCVAR_DONTRECORD, "Change ImGui debug display mode: 0 - none, 1 - full, 2 - overlay");
}

ON_DLL_LOAD("vguimatsurface.dll", ImGuiVGui, (CModule module))
{
	VGUI_Surface031 = Sys_GetFactoryPtr("vguimatsurface.dll", "VGUI_Surface031").RCast<ISurface*>();

	auto* setCursor = VGUI_Surface031->m_vtable->SetCursor;
	MH_CreateHook(setCursor, h_setCursor, (void**)&o_pSetCursor);
	MH_EnableHook(setCursor);

	auto* lockCursor = VGUI_Surface031->m_vtable->LockCursor;
	MH_CreateHook(lockCursor, h_lockCursor, (void**)&o_pLockCursor);
	MH_EnableHook(lockCursor);
}
