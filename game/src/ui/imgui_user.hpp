#pragma once

#include <imgui.h>

#include <string>
#include <vector>

namespace ImGui {

IMGUI_API bool InputText(const char *label, std::string &buf, ImGuiInputTextFlags flags=0);
IMGUI_API bool InputTextMultiline(const char *label, std::string &buf, const ImVec2 &size=ImVec2(0,0), ImGuiInputTextFlags flags=0);
IMGUI_API bool Combo(const char *label, int &idx, const std::vector<std::string> &lst, int popup_max_height_in_items=-1);
IMGUI_API bool Input(const char *label, int &v, int step=1, int step2=100, ImGuiInputTextFlags flags=0);
IMGUI_API bool InputClamp(const char *label, int &v, int min, int max, int step=1, int step2=100, ImGuiInputTextFlags flags=0);

IMGUI_API void Tooltip(const char *str, float width=30.0f);

IMGUI_API void TextUnformatted(const char *str, int halign, bool wrap);
IMGUI_API bool Button(const char *label, int halign, const ImVec2 &size = ImVec2(0, 0));

}
