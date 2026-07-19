#ifndef AOE_UI_MENULABEL_CPP
#define AOE_UI_MENULABEL_CPP 1

#include "../ui.hpp"
#include "fullscreenmenu.hpp"

namespace aoe {

namespace ui {

extern void reshapeRelative(SDL_FRect &bnds, const ImVec2 &sz, const MenuButtonLayoutRelative &r);

MenuLabel::MenuLabel(const MenuButtonLayoutRelative &layout, const char *name, TextHalign halign)
	: bnds{ 0, 0, 0, 0 }, name(name), layout(layout), halign(halign) {}

void MenuLabel::reshape(ImGuiViewport *vp) {
	reshapeRelative(bnds, vp->WorkSize, layout);
}

void MenuLabel::show(Frame &f) const {
	ImGui::SetCursorPosY(bnds.y);

	SDL_Rect ibnds{ bnds.x, bnds.y, bnds.w, bnds.h };
	ImU32 rgba = IM_COL32_WHITE;

	DrawTextShadow(ibnds, name, rgba, halign, true);
}

}

}

#endif