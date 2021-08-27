#include "all.hpp"

#include <atomic>

extern std::atomic_bool running;

namespace genie {
namespace ui {

void TexturesWidget::display(Texture &tex) {
	ImVec2 sz(300, 200);
	ImGui::SetNextWindowSizeConstraints(sz, ImVec2(FLT_MAX, FLT_MAX));

	ImGui::Begin("Texture map");
	{
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGui::BeginChild("scrolling", sz, false, ImGuiWindowFlags_HorizontalScrollbar);
		{
			ImTextureID id = (ImTextureID)tex.tex;
			ImGui::Image(id, ImVec2(tex.ts.bnds.w, tex.ts.bnds.h));
		}
		ImGui::EndChild();
	}
	ImGui::End();
}

DebugUI::DebugUI() : show_gfx(false), tex() {}

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
		ImGui::Checkbox("Graphics debugger", &show_gfx);
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Settings")) {
		ImGui::EndMenu();
	}

	ImGui::EndMainMenuBar();
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
