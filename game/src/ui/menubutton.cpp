#ifndef AOE_UI_MENUBUTTON_HPP
#define AOE_UI_MENUBUTTON_HPP 1

#include "../ui.hpp"
#include "../legacy/limits.hpp"
#include "../engine/sdl.hpp"

namespace aoe {
namespace ui {

MenuButton::MenuButton(float y, const char *name, const char *tooltip, unsigned state)
	: bnds{ 0, 0, 0, 0 }
	, name(name), tooltip(tooltip), state(state)
	, type(MenuButtonLayoutType::vertical), layout(y) {}

MenuButton::MenuButton(const MenuButtonLayoutCorner &c, const char *name)
	: bnds{ 0, 0, 0, 0 }
	, name(name), tooltip(NULL), state(0)
	, type(MenuButtonLayoutType::corner), layout(c) {}

static void reshapeVertical(SDL_FRect &bnds, const ImVec2 &sz, float relY)
{
	float w = sz.x, h = sz.y;

	bnds.x = 212.0f / WINDOW_WIDTH_MIN * w;
	bnds.w = (586.0f - 212.0f) / WINDOW_WIDTH_MIN * w;
	bnds.y = relY / href * h;
	bnds.h = 64.0f / href * h;
}

static void reshapeCorner(SDL_FRect &bnds, const ImVec2 &sz, const MenuButtonLayoutCorner &c)
{
	float w = vp->WorkSize.x, h = vp->WorkSize.y;
	float margin = c.margin;
	float ws = w / WINDOW_WIDTH_MIN, hs = h / WINDOW_HEIGHT_MIN;

	bnds.w = (float)c.w * ws;
	bnds.h = (float)c.h * hs;

	bnds.x = w - bnds.w - margin * ws;
	bnds.y = c.topRight ? margin * hs : h - bnds.h - margin * hs;
}

void MenuButton::reshape(ImGuiViewport *vp) {
	float w = vp->WorkSize.x, h = vp->WorkSize.y;

	switch (type) {
	case MenuButtonLayoutType::vertical:
		reshapeVertical(bnds, vp->WorkSize, layout.relY);
		break;
	case MenuButtonLayoutType::corner:
		reshapeCorner(bnds, vp->WorkPos, layout.corner);
		break;
	}
}

bool MenuButton::show(Frame &f, Audio &sfx, const BackgroundColors &col) const {
	ImGui::SetCursorPosY(bnds.y);

	float x0 = bnds.x, y0 = bnds.y;
	float x1 = x0 + bnds.w, y1 = y0 + bnds.h;

	FillRect(x0, y0, x1, y1, SDL_Color{ 0, 0, 0, 127 });
	if (state & (unsigned)MenuButtonState::active)
		DrawBorderInv(x0, y0, x1, y1, col);
	else
		DrawBorder(x0, y0, x1, y1, col);

	SDL_Rect ibnds{ bnds.x, bnds.y, bnds.w, bnds.h }; // Integer BouNDarieS
	ImU32 rgba = IM_COL32_WHITE;
	if (state & (unsigned)MenuButtonState::selected)
		rgba = IM_COL32(255, 255, 0, 255);

	if (state & (unsigned)MenuButtonState::disabled)
		rgba = IM_COL32(63, 63, 63, 255);

	DrawTextShadow(ibnds, name, rgba, TextHalign::center, true);
	return false;
}

}
}

#endif