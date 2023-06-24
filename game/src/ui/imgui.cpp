#include "imgui_user.hpp"

#include <cstdarg>

#include <imgui_internal.h>

#ifndef IMGUI_DEFINE_MATH_OPERATORS
IM_MSVC_RUNTIME_CHECKS_OFF
static inline ImVec2 operator*(const ImVec2 &lhs, const float rhs)              { return ImVec2(lhs.x * rhs, lhs.y * rhs); }
static inline ImVec2 operator/(const ImVec2 &lhs, const float rhs)              { return ImVec2(lhs.x / rhs, lhs.y / rhs); }
static inline ImVec2 operator+(const ImVec2 &lhs, const ImVec2 &rhs)            { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
static inline ImVec2 operator-(const ImVec2 &lhs, const ImVec2 &rhs)            { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }
static inline ImVec2 operator*(const ImVec2 &lhs, const ImVec2 &rhs)            { return ImVec2(lhs.x * rhs.x, lhs.y * rhs.y); }
static inline ImVec2 operator/(const ImVec2 &lhs, const ImVec2 &rhs)            { return ImVec2(lhs.x / rhs.x, lhs.y / rhs.y); }
static inline ImVec2 &operator*=(ImVec2 &lhs, const float rhs)                  { lhs.x *= rhs; lhs.y *= rhs; return lhs; }
static inline ImVec2 &operator/=(ImVec2 &lhs, const float rhs)                  { lhs.x /= rhs; lhs.y /= rhs; return lhs; }
static inline ImVec2 &operator+=(ImVec2 &lhs, const ImVec2 &rhs)                { lhs.x += rhs.x; lhs.y += rhs.y; return lhs; }
static inline ImVec2 &operator-=(ImVec2 &lhs, const ImVec2 &rhs)                { lhs.x -= rhs.x; lhs.y -= rhs.y; return lhs; }
static inline ImVec2 &operator*=(ImVec2 &lhs, const ImVec2 &rhs)                { lhs.x *= rhs.x; lhs.y *= rhs.y; return lhs; }
static inline ImVec2 &operator/=(ImVec2 &lhs, const ImVec2 &rhs)                { lhs.x /= rhs.x; lhs.y /= rhs.y; return lhs; }
static inline ImVec4 operator+(const ImVec4 &lhs, const ImVec4 &rhs)            { return ImVec4(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w); }
static inline ImVec4 operator-(const ImVec4 &lhs, const ImVec4 &rhs)            { return ImVec4(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w); }
static inline ImVec4 operator*(const ImVec4 &lhs, const ImVec4 &rhs)            { return ImVec4(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w); }
IM_MSVC_RUNTIME_CHECKS_RESTORE
#endif

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

void TextEx(const char *text, int halign, const char *text_end=NULL, ImGuiTextFlags flags=0)
{
	ImGuiWindow *window = GetCurrentWindow();
	if (window->SkipItems)
		return;
	ImGuiContext &g = *GImGui;

	// Accept null ranges
	if (text == text_end)
		text = text_end = "";

	// Calculate length
	const char *text_begin = text;
	if (text_end == NULL)
		text_end = text + strlen(text); // FIXME-OPT

	ImVec2 text_pos(window->DC.CursorPos.x, window->DC.CursorPos.y + window->DC.CurrLineTextBaseOffset);
	const float wrap_pos_x = window->DC.TextWrapPos;
	const bool wrap_enabled = (wrap_pos_x >= 0.0f);

	// Common case
	const float wrap_width = wrap_enabled ? CalcWrapWidthForPos(window->DC.CursorPos, wrap_pos_x) : 0.0f;
	const ImVec2 text_size = CalcTextSize(text_begin, text_end, false, wrap_width);

	// now adjust to text alignment
	// TODO this breaks for (multiple) wrapped lines
	switch (halign) {
	case 0:
		// left alignment, do nothing
		break;
	case 1:
		// center alignment
		text_pos.x += (window->SizeFull.x - window->Pos.x - window->WindowPadding.x - text_size.x) / 2;
		break;
	case 2:
		// right alignment
		text_pos.x = window->SizeFull.x - window->Pos.x - text_size.x - window->WindowPadding.x;
		break;
	}

	ImRect bb(text_pos, text_pos + text_size);
	ItemSize(text_size, 0.0f);
	if (!ItemAdd(bb, 0))
		return;

	// Render (we don't hide text after ## in this end-user function)
	RenderTextWrapped(bb.Min, text_begin, text_end, wrap_width);
}

void TextV(const char *fmt, int halign, va_list args)
{
	ImGuiWindow *window = GetCurrentWindow();
	if (window->SkipItems)
		return;

	// FIXME-OPT: Handle the %s shortcut?
	const char *text, *text_end;
	ImFormatStringToTempBufferV(&text, &text_end, fmt, args);
	TextEx(text, halign, text_end, ImGuiTextFlags_NoWidthForLargeClippedText);
}

void TextWrappedV(const char *fmt, int halign, va_list args)
{
	ImGuiContext &g = *GImGui;
	bool need_backup = (g.CurrentWindow->DC.TextWrapPos < 0.0f);  // Keep existing wrap position if one is already set
	if (need_backup)
		PushTextWrapPos(0.0f);
	if (fmt[0] == '%' && fmt[1] == 's' && fmt[2] == 0)
		TextEx(va_arg(args, const char*), NULL, ImGuiTextFlags_NoWidthForLargeClippedText); // Skip formatting
	else
		TextV(fmt, args);
	if (need_backup)
		PopTextWrapPos();
}

void TextWrapped(const char *fmt, int halign, ...)
{
	va_list args;
	va_start(args, fmt);
	TextWrappedV(fmt, halign, args);
	va_end(args);
}

void TextUnformatted(const char *str, int halign, bool wrap)
{
	TextEx(str, halign, NULL, 0);
}

static bool ButtonEx(const char *label, int halign, const ImVec2 &size_arg, ImGuiButtonFlags flags)
{
	ImGuiWindow *window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext &g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 label_size = CalcTextSize(label, NULL, true);

	ImVec2 pos = window->DC.CursorPos;

	if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
		pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;
	ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

	// now adjust to text alignment
	// TODO this breaks for (multiple) wrapped lines
	switch (halign) {
	case 0:
		// left alignment, do nothing
		break;
	case 1:
		// center alignment
		pos.x += (window->SizeFull.x - window->Pos.x - window->WindowPadding.x - size.x) / 2;
		break;
	case 2:
		// right alignment
		pos.x = window->SizeFull.x - window->Pos.x - size.x - window->WindowPadding.x;
		break;
	}

	const ImRect bb(pos, pos + size);
	ItemSize(size, style.FramePadding.y);
	if (!ItemAdd(bb, id))
		return false;

	if (g.LastItemData.InFlags & ImGuiItemFlags_ButtonRepeat)
		flags |= ImGuiButtonFlags_Repeat;

	bool hovered, held;
	bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

	// Render
	const ImU32 col = GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
	RenderNavHighlight(bb, id);
	RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);

	if (g.LogEnabled)
		LogSetNextTextDecoration("[", "]");
	RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);

	// Automatically close popups
	//if (pressed && !(flags & ImGuiButtonFlags_DontClosePopups) && (window->Flags & ImGuiWindowFlags_Popup))
	//    CloseCurrentPopup();

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
	return pressed;
}

bool Button(const char *label, int halign, const ImVec2 &size_arg)
{
	return ButtonEx(label, halign, size_arg, ImGuiButtonFlags_None);
}

}
