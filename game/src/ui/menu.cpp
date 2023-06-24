#include "../ui.hpp"

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>

#include <utility>

#include <minmax.hpp>

namespace aoe {

namespace ui {

Menu::Menu() : open(false) {}

Menu::~Menu() {
	if (open) close();
}

Menu::Menu(Menu &&m) noexcept : open(std::exchange(m.open, false)) {}

void Menu::close() {
	assert(open);
	ImGui::EndMenu();
	open = false;
}

bool Menu::begin(const char *str_id) {
	assert(!open);
	return open = ImGui::BeginMenu(str_id);
}

bool Menu::item(const char *item_id) {
	assert(open);
	return ImGui::MenuItem(item_id);
}

bool Menu::chkbox(const char *s, bool &b) {
	return ImGui::Checkbox(s, &b);
}

bool Menu::btn(const char *s, const ImVec2 &sz) {
	return ImGui::Button(s, sz);
}

}

}
