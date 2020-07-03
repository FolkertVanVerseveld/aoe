/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include <SDL2/SDL_pixels.h>

namespace ImGui {

IMGUI_API void PixelBox(ImU32 col, const ImVec2 &size = ImVec2(0, 0));
IMGUI_API void ColorPalette(SDL_Palette *pal, const ImVec2 &pxsize = ImVec2(0, 0));

}
