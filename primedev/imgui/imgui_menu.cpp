#include "imgui_menu.h"
#include "imgui/imgui.h"

ImGuiMenu::ImGuiMenu(const char* name, ImGuiRenderCallback callback, const char* shortcut = nullptr)
	: m_name(name)
	, m_callback(callback)
	, m_shortcut(shortcut)
{
}

void ImGuiMenu::Render()
{
	if (!m_isVisible || m_callback == nullptr)
		return;

	ImGui::Begin(m_name, &m_isVisible);

	m_callback();

	ImGui::End();
}

bool ImGuiMenu::RenderMenuItem()
{
	return ImGui::MenuItem(m_name, m_shortcut, &m_isVisible);
}
