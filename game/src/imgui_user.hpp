#pragma once

#include <imgui.h>

#include <string>
#include <vector>

namespace ImGui {

IMGUI_API bool InputText(const char *label, std::string &buf, ImGuiInputTextFlags flags=0);
IMGUI_API bool InputTextMultiline(const char *label, std::string &buf, const ImVec2 &size=ImVec2(0,0), ImGuiInputTextFlags flags=0);
IMGUI_API bool Combo(const char *label, int &idx, const std::vector<std::string> &lst, int popup_max_height_in_items=-1);

IMGUI_API void Tooltip(const char *str, float width=35.0f);

}
