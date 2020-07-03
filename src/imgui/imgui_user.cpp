/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */
#define IMGUI_DEFINE_MATH_OPERATORS 1

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_user.h"

namespace ImGui {

IMGUI_API void PixelBox(ImU32 col, const ImVec2 &size) {
	ImGuiWindow *window = GetCurrentWindow();
	if (window->SkipItems)
		return;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const float line_height = ImMax(ImMin(window->DC.CurrLineSize.y, g.FontSize + g.Style.FramePadding.y*2), g.FontSize);
	const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(g.FontSize, line_height));
	ItemSize(bb);
	if (!ItemAdd(bb, 0))
	{
		SameLine(0, style.FramePadding.x);
		return;
	}

	// Render and stay on same line
	ImU32 text_col = GetColorU32(ImGuiCol_Text);
	window->DrawList->AddRectFilled(bb.Min, bb.Max, col);
	SameLine(0, style.FramePadding.x);
}

IMGUI_API void ColorPalette(SDL_Palette *pal, const ImVec2 &pxsize) {
	for (unsigned y = 0, i = 0, n = pal->ncolors; y < 16; ++y) {
		for (unsigned x = 0; x < 16 && i < n; ++x, ++i)
			ImGui::PixelBox(IM_COL32(pal->colors[i].r, pal->colors[i].g, pal->colors[i].b, pal->colors[i].a), pxsize);
		ImGui::NewLine();
	}
}

}
