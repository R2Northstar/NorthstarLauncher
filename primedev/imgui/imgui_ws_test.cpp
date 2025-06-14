#include "imgui_ws_test.h"
#include "imgui-ws/imgui-ws.h"
#include "imgui-ws/imgui-draw-data-compressor.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_dx11.h"
#include "imgui/backends/imgui_impl_win32.h"

#include "engine/r2engine.h"

AUTOHOOK_INIT()

static ImGuiIO* io = nullptr;
static ImGuiWS imguiWS;
static bool isInited = false;
static bool thing = true;

static ID3D11Device** device = nullptr; 
static ID3D11DeviceContext** deviceContext = nullptr;

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

	return ImGuiKey_COUNT;
}

struct State
{
	State() {}

	bool showDemoWindow = true;

	// client control management
	struct ClientData
	{
		bool hasControl = false;

		std::string ip = "---";
	};

	// client control
	float tControl_s = 10.0f;
	float tControlNext_s = 0.0f;

	int controlIteration = 0;
	int curIdControl = -1;
	std::map<int, ClientData> clients;

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

	// client input
	std::vector<InputEvent> inputEvents;
	std::string lastAddText = "";

	void handle(ImGuiWS::Event&& event);
	void update();
};

static State state;

void State::handle(ImGuiWS::Event&& event)
{
	switch (event.type)
	{
	case ImGuiWS::Event::Connected:
	{
		clients[event.clientId].ip = event.ip;
	}
	break;
	case ImGuiWS::Event::Disconnected:
	{
		clients.erase(event.clientId);
	}
	break;
	case ImGuiWS::Event::MouseMove:
	{
		if (event.clientId == curIdControl)
		{
			inputEvents.push_back(
				InputEvent {InputEvent::Type::EMousePos, false, ImGuiKey_COUNT, -1, {event.mouse_x, event.mouse_y}, 0.0f, 0.0f});
		}
	}
	break;
	case ImGuiWS::Event::MouseDown:
	{
		if (event.clientId == curIdControl)
		{
			// map the JS button code to Dear ImGui's button code
			ImGuiMouseButton butImGui = event.mouse_but;
			switch (event.mouse_but)
			{
			case 1:
				butImGui = ImGuiMouseButton_Middle;
				break;
			case 2:
				butImGui = ImGuiMouseButton_Right;
				break;
			}

			inputEvents.push_back(
				InputEvent {InputEvent::Type::EMouseButton, true, ImGuiKey_COUNT, butImGui, {event.mouse_x, event.mouse_y}, 0.0f, 0.0f});
		}
	}
	break;
	case ImGuiWS::Event::MouseUp:
	{
		if (event.clientId == curIdControl)
		{
			// map the JS button code to Dear ImGui's button code
			ImGuiMouseButton butImGui = event.mouse_but;
			switch (event.mouse_but)
			{
			case 1:
				butImGui = ImGuiMouseButton_Middle;
				break;
			case 2:
				butImGui = ImGuiMouseButton_Right;
				break;
			}

			inputEvents.push_back(
				InputEvent {InputEvent::Type::EMouseButton, false, ImGuiKey_COUNT, butImGui, {event.mouse_x, event.mouse_y}, 0.0f, 0.0f});
		}
	}
	break;
	case ImGuiWS::Event::MouseWheel:
	{
		if (event.clientId == curIdControl)
		{
			inputEvents.push_back(InputEvent {InputEvent::Type::EMouseWheel, false, ImGuiKey_COUNT, -1, {}, event.wheel_x, event.wheel_y});
		}
	}
	break;
	case ImGuiWS::Event::KeyUp:
	{
		if (event.clientId == curIdControl)
		{
			if (event.key > 0)
			{
				ImGuiKey keyImGui = ::toImGuiKey(event.key);
				inputEvents.push_back(InputEvent {InputEvent::Type::EKey, false, keyImGui, -1, {}, 0.0f, 0.0f});
			}
		}
	}
	break;
	case ImGuiWS::Event::KeyDown:
	{
		if (event.clientId == curIdControl)
		{
			if (event.key > 0)
			{
				ImGuiKey keyImGui = ::toImGuiKey(event.key);
				inputEvents.push_back(InputEvent {InputEvent::Type::EKey, true, keyImGui, -1, {}, 0.0f, 0.0f});
			}
		}
	}
	break;
	case ImGuiWS::Event::KeyPress:
	{
		if (event.clientId == curIdControl)
		{
			lastAddText.resize(1);
			lastAddText[0] = event.key;
		}
	}
	break;
	default:
	{
		printf("Unknown input event\n");
	}
	}
}

void State::update()
{
	if (clients.size() > 0 && (clients.find(curIdControl) == clients.end() || ImGui::GetTime() > tControlNext_s))
	{
		if (clients.find(curIdControl) != clients.end())
		{
			clients[curIdControl].hasControl = false;
		}
		int k = ++controlIteration % clients.size();
		auto client = clients.begin();
		std::advance(client, k);
		client->second.hasControl = true;
		curIdControl = client->first;
		tControlNext_s = ImGui::GetTime() + tControl_s;
		ImGui::GetIO().ClearInputKeys();
	}

	if (clients.size() == 0)
	{
		curIdControl = -1;
	}

	if (curIdControl > 0)
	{
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
			}
		}

		inputEvents.clear();
		lastAddText = "";
	}
}

void ImGuiDisplay::Start()
{
	int port = 5000;

	IMGUI_CHECKVERSION();
	m_context = ImGui::CreateContext();
	// ImGui::GetIO().MouseDrawCursor = true;

	ImGui_ImplWin32_Init(FindWindow(L"Titanfall 2", NULL));
	ImGui_ImplDX11_Init(*device, *deviceContext);

	ImGui::StyleColorsDark();
	ImGui::GetStyle().AntiAliasedFill = false;
	ImGui::GetStyle().AntiAliasedLines = false;
	ImGui::GetStyle().WindowRounding = 0.0f;
	ImGui::GetStyle().ScrollbarRounding = 0.0f;

	// setup imgui-ws
	imguiWS.init(port, "./", {""});

	// prepare font texture
	{
		unsigned char* pixels;
		int width, height;
		ImGui::GetIO().Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);
		imguiWS.setTexture(0, ImGuiWS::Texture::Type::Alpha8, width, height, (const char*)pixels);
	}

	isInited = true;
}

void ImGuiDisplay::Render(float deltaTime)
{
	if (!isInited)
		Start();

	// websocket event handling
	auto events = imguiWS.takeEvents();
	for (auto& event : events)
	{
		state.handle(std::move(event));
	}
	state.update();

	{
		auto& io = ImGui::GetIO();
		io.DisplaySize = ImVec2(2560, 1440);
	}

	ImGui::SetCurrentContext(m_context);
	ImGui_ImplDX11_NewFrame();
	//ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	bool isOverlay = debugVisible && debugOverlay;
	ImGui::PushStyleVar(ImGuiStyleVar_Alpha, isOverlay ? 0.75f : 1.f);

	for (auto& menu : m_menus)
		menu.Render();

	// dear imgui demo. please never delete this.
	if (state.showDemoWindow)
		ImGui::ShowDemoWindow(&state.showDemoWindow);

	// main menu bar
	if (ImGui::BeginMainMenuBar())
	{
		ImGui::MenuItem("DearImGui Demo", 0, &state.showDemoWindow);
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
	if (debugVisible)
		ImGui_ImplDX11_RenderDrawData(drawData);
}

void ImGuiDisplay::Shutdown()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext(m_context);
	isInited = false;
}

void ImGuiDisplay::RegisterMenu(const char* name, ImGuiRenderCallback callback, const char* shortcut)
{
	m_menus.emplace_back(name, callback, shortcut);
}

ImGuiDisplay& ImGuiDisplay::GetInstance()
{
	static ImGuiDisplay* display = nullptr;

	if (display == nullptr)
		display = new ImGuiDisplay();

	return *display;
}

// todo: replace with proper Present hook
AUTOHOOK(Thing3, materialsystem_dx11.dll + 0x54340, __int64, , (__int64 a1, __int64 a2, __int64 a3, int a4))
{
	//spdlog::info("THING3");
	ImGuiDisplay::GetInstance().Render(0.05f);
	return Thing3(a1, a2, a3, a4);
}

ON_DLL_LOAD("materialsystem_dx11.dll", ImGuiMaterialSystem, (CModule module))
{
	AUTOHOOK_DISPATCH_MODULE(materialsystem_dx11.dll)

	device = module.Offset(0x14E8DD0).RCast<ID3D11Device**>();
	deviceContext = module.Offset(0x14E8DD8).RCast<ID3D11DeviceContext**>();
}

static bool IsKeyMsg(UINT uMsg)
{
	return uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST;
}

static bool IsMouseMsg(UINT uMsg)
{
	return uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static int (*o_pGameWndProc)(void* game, const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) = nullptr;
static int h_GameWndProc(void* game, const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	auto& instance = ImGuiDisplay::GetInstance();
	// only handle input if not in overlay mode
	if (ImGui::GetCurrentContext() != nullptr && instance.IsDebugVisible() && !instance.IsDebugOverlay())
	{
		ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

		auto& io = ImGui::GetIO();
		// Only block from game if imgui capturing
		if (IsMouseMsg(uMsg) && io.WantCaptureMouse)
		{
			return 0;
		}

		if (IsKeyMsg(uMsg) && io.WantCaptureKeyboard)
		{
			return 0;
		}
	}

	return o_pGameWndProc(game, hWnd, uMsg, wParam, lParam);
}

static void ConCommand_toggleimgui(const CCommand& arg)
{
	NOTE_UNUSED(arg);
	auto& instance = ImGuiDisplay::GetInstance();
	if (instance.IsDebugOverlay())
	{
		instance.SetDebugVisible(false);
		instance.SetDebugOverlay(false);
	}
	else if (instance.IsDebugVisible())
	{
		instance.SetDebugOverlay(true);
	}
	else
	{
		instance.SetDebugVisible(true);
	}
}

ON_DLL_LOAD_RELIESON("client.dll", ImGuiClient, ConCommand, (CModule module))
{
	RegisterConCommand("toggleimgui", ConCommand_toggleimgui, "Show/Overlay/Hide the ImGui debug.", FCVAR_DONTRECORD);
}

ON_DLL_LOAD("inputsystem.dll", ImGuiInput, (CModule module))
{
	o_pGameWndProc = module.Offset(0x8B80).RCast<decltype(o_pGameWndProc)>();
	HookAttach(&(PVOID&)o_pGameWndProc, h_GameWndProc);
}
