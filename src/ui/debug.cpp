#include "all.hpp"

#include "../engine.hpp"

#include <atomic>

extern std::atomic_bool running;

namespace genie {

extern AoE *eng;

namespace ui {

void TexturesWidget::display() {
	if (!visible)
		return;

	Texture &tex = eng->tex_bkg;
	ImVec2 sz(300, 200);
	ImGui::SetNextWindowSizeConstraints(sz, ImVec2(FLT_MAX, FLT_MAX));

	ImGui::Begin("Texture map", &visible);
	{
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
		{
			ImTextureID id = (ImTextureID)tex.tex;
			ImGui::Image(id, ImVec2(tex.ts.bnds.w, tex.ts.bnds.h));
		}
		ImGui::EndChild();
	}
	ImGui::End();
}

DebugUI::DebugUI() : tex() {}

void DebugUI::display() {
	if (!ImGui::BeginMainMenuBar())
		return;

	if (ImGui::BeginMenu("File")) {
		if (ImGui::MenuItem("About")) {}

		ImGui::Separator();

		if (ImGui::MenuItem("Quit"))
			running = false;

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("View")) {
		ImGui::Checkbox("Graphics debugger", &tex.visible);
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Settings")) {
		ImGui::EndMenu();
	}

	ImGui::EndMainMenuBar();

	tex.display();
}

UI::UI() : m_show_debug(false), dbg() {}

void UI::show_debug(bool enabled) {
	m_show_debug = enabled;
}

void UI::display() {
	if (m_show_debug)
		dbg.display();
}

}
}
