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
	bool IsDebugVisible() const { return debugVisible; }
	void SetDebugVisible(bool isVisible) { debugVisible = isVisible; }
	bool IsDebugOverlay() const { return debugVisible && debugOverlay; }
	void SetDebugOverlay(bool isOverlay) { debugOverlay = isOverlay; }

	static ImGuiDisplay& GetInstance();

	ImGuiDisplay(const ImGuiDisplay&) = delete;
	ImGuiDisplay(ImGuiDisplay&&) = delete;

private:
	// not instantiable outside of GetInstance()
	ImGuiDisplay() = default;

	std::vector<ImGuiMenu> m_menus = {};
	ImGuiContext* m_context = nullptr;
	bool debugVisible = false;
	bool debugOverlay = false;
};
