#include "font.hpp"

#include <cstddef>
#include <cstdio>
#include <cmath>

#include <imgui.h>

#include "../engine.hpp"

#if _WIN32
#include <WinBase.h>

static std::string get_username() {
	std::string name(256, ' ');
	DWORD size = name.size();

	while (!GetUserName(name.data(), &size)) {
		DWORD err = GetLastError();

		if (err != ERROR_INSUFFICIENT_BUFFER)
			throw std::runtime_error(std::string("get_username failed: code ") + std::to_string(err));

		name.resize(size = name.size() * 2, ' ');
	}

	if (size)
		name.resize(size - 1);

	return name;
}
#else
#include <unistd.h>

static std::string get_username() {
	std::string name(getlogin());
	return name;
}
#endif

namespace aoe {

FontGuard::FontGuard(ImFont *fnt) {
	ImGui::PushFont(fnt);
}

FontGuard::~FontGuard() {
	ImGui::PopFont();
}

FontCache::FontCache() : fnt_arial(NULL), fnt_copper(NULL), fnt_copper2(NULL) {}

bool FontCache::loaded() const noexcept {
	return fnt_arial && fnt_copper && fnt_copper2;
}

static ImFont *try_add_font(ImFontAtlas *a, const char *path, float size) {
	FILE *f;

	if (!(f = fopen(path, "rb")))
		return NULL;

	fclose(f);

	return a->AddFontFromFileTTF(path, size);
}

bool FontCache::try_load() {
	ImGuiIO &io = ImGui::GetIO();
	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
	//ImFont *font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != NULL);
#if _WIN32
	float f = SDL::fnt_scale;
	fnt_arial = try_add_font(io.Fonts, "C:\\Windows\\Fonts\\arial.ttf", ceil(13.0f * f));

	if (!(fnt_copper = try_add_font(io.Fonts, "C:\\Windows\\Fonts\\COPRGTB.TTF", ceil(18.0f * f)))) {
		std::string localpath(std::string("C:\\Users\\") + get_username() + "\\AppData\\Local\\Microsoft\\Windows\\Fonts\\COPRGTB.TTF");

		fnt_copper = try_add_font(io.Fonts, localpath.c_str(), ceil(18.0f * f));
	}

	if (!(fnt_copper2 = try_add_font(io.Fonts, "C:\\Windows\\Fonts\\COPRGTL.TTF", ceil(30.0f * f)))) {
		std::string localpath(std::string("C:\\Users\\") + get_username() + "\\AppData\\Local\\Microsoft\\Windows\\Fonts\\COPRGTL.TTF");

		fnt_copper2 = try_add_font(io.Fonts, localpath.c_str(), ceil(30.0f * f));
	}
#else
	float f = SDL::fnt_scale;

	#define FONT_DIR "/usr/share/fonts/truetype/"

	fnt_arial = try_add_font(io.Fonts, FONT_DIR "liberation/LiberationSans-Regular.ttf", ceil(15.0f * f));

	fnt_copper = try_add_font(io.Fonts, FONT_DIR "abyssinica/AbyssinicaSIL-Regular.ttf", ceil(18.0f * f));
	fnt_copper2 = try_add_font(io.Fonts, FONT_DIR "ancient-scripts/Symbola_hint.ttf", ceil(30.0f * f));
#endif
	return loaded();
}

}
