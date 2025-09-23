#pragma once

#include <vector>
#include "imgui_menu.h"
#include "imgui_club/imgui_threaded_rendering/imgui_threaded_rendering.h"

class ImGuiDisplay
{
public:
	void Start();
	void Render();
	void Shutdown();

	void RegisterMenu(const char* name, ImGuiRenderCallback callback, const char* shortcut);

	static ImGuiDisplay& GetInstance();

	ImGuiDisplay(const ImGuiDisplay&) = delete;
	ImGuiDisplay(ImGuiDisplay&&) = delete;

private:
	// not instantiable outside of GetInstance()
	ImGuiDisplay() = default;

	std::vector<ImGuiMenu> m_menus = {};
	ImGuiContext* m_context = nullptr;
	// demo window is built into the display
	bool m_showDemoWindow = false;
};

void RenderScriptThing(const char* message);
