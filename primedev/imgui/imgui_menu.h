#pragma once
#include <string>

using ImGuiRenderCallback = void (*)();

class ImGuiMenu
{
private:
	const char* m_name = "";
	const char* m_shortcut = "";
	bool m_isVisible = false;
	ImGuiRenderCallback m_callback;

public:
	ImGuiMenu(const char* name, ImGuiRenderCallback callback, const char* shortcut);

	void Render();
	bool RenderMenuItem();
};
