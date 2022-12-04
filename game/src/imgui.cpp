#include "imgui_user.hpp"

namespace ImGui {

static int input_text_resize(ImGuiInputTextCallbackData *data)
{
	if (data->EventFlag != ImGuiInputTextFlags_CallbackResize)
		return 0;

	std::string *s = (std::string*)data->UserData;
	IM_ASSERT(s->data() == data->Buf);
	s->resize(data->BufSize);
	data->Buf = s->data();

	return 0;
}

bool InputText(const char *label, std::string &buf, ImGuiInputTextFlags flags)
{
	/*
	 * ImGui assumes the buffer is '\0' terminated. This is really annoying,
	 * as we have to add one to prevent ImGui messing with our string, and we
	 * have to remove it after our InputText call as we want a proper C++ string.
	 */

	// make imgui happy
	if (!buf.empty() && buf.back() != '\0')
		buf.push_back('\0');

	bool v = InputText(label, buf.data(), buf.size(), flags | ImGuiInputTextFlags_CallbackResize, input_text_resize, &buf);

	// revert imgui mess
	if (!buf.empty() && buf.back() == '\0')
		buf.pop_back();

	if (v) {
		size_t pos = buf.find_last_of('\0');
		if (pos != std::string::npos)
			buf.erase(pos);
	}

	return v;
}

bool InputTextMultiline(const char *label, std::string &buf, const ImVec2 &size, ImGuiInputTextFlags flags)
{
	// make imgui happy
	if (!buf.empty() && buf.back() != '\0')
		buf.push_back('\0');

	bool v = InputTextMultiline(label, buf.data(), buf.size(), size, flags | ImGuiInputTextFlags_CallbackResize, input_text_resize, &buf);

	// revert imgui mess
	if (!buf.empty() && buf.back() == '\0')
		buf.pop_back();

	if (v) {
		size_t pos = buf.find_last_of('\0');
		if (pos != std::string::npos)
			buf.erase(pos);
	}

	return false;
}

/* Wrapper to get combo elements from c++ vector<string>. */
static bool vec_get(void *data, int idx, const char **out)
{
	const std::vector<std::string> *lst = (decltype(lst))data;

	if (idx < 0 || idx >= lst->size())
		return false;

	*out = (*lst)[idx].c_str();
	return true;
}

bool Combo(const char *label, int &idx, const std::vector<std::string> &lst, int popup_max_height_in_items)
{
	return ImGui::Combo(label, &idx, vec_get, (void*)&lst, lst.size(), popup_max_height_in_items);
}

void Tooltip(const char *str, float width)
{
	ImGui::BeginTooltip();
	ImGui::PushTextWrapPos(ImGui::GetFontSize() * width);
	ImGui::TextUnformatted(str);
	ImGui::PopTextWrapPos();
	ImGui::EndTooltip();
}

}
