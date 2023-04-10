#include "../ui.hpp"

#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_opengl3.h>

#include <minmax.hpp>

namespace aoe {

namespace ui {

MenuBar::MenuBar() : open(ImGui::BeginMainMenuBar()) {}

MenuBar::~MenuBar() {
	if (open) close();
}

void MenuBar::close() {
	assert(open);
	ImGui::EndMainMenuBar();
	open = false;
}

}

}
