#pragma once

#include <vector>
#include "imgui_menu.h"

class ImGuiDisplay
{
public:
	void Start();
	void Render(float deltaTime);
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
};
