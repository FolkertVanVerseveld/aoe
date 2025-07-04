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

FontCache fnt;

FontGuard::FontGuard(const Font &fnt) {
	ImGui::PushFont(fnt.fnt);
}

FontGuard::~FontGuard() {
	ImGui::PopFont();
}

FontCache::FontCache() : arial(13.0f), copper(22.0f), copper2(36.0f) {}

bool FontCache::loaded() const noexcept {
	return arial && copper && copper2;
}

static ImFont *try_add_font(ImFontAtlas *a, const char *path, float size) {
	FILE *f;
	void *blob = NULL;
	ImFont *fnt = NULL;
	ImFontConfig cfg;

	if (!(f = fopen(path, "rb")))
		return NULL;

	fseek(f, 0, SEEK_END);
	long fend = ftell(f);
	fseek(f, 0, SEEK_SET);
	long fstart = ftell(f);

	long fsize = fend - fstart;
	if (fsize < 0 || !(blob = malloc(fsize)) || fread(blob, fsize, 1, f) != 1)
		goto fail;

	// make sure it is copied by ImGui, so we can free and don't have to leak memory
	cfg.FontDataOwnedByAtlas = false;

	if (blob)
		fnt = a->AddFontFromMemoryTTF(blob, fsize, size, &cfg);

fail:
	fclose(f);
	free(blob);

	return fnt;
}

Font::Font(float pt) : fnt(NULL), path(), pt(pt) {}

bool Font::load(ImFontAtlas *fa, const char *path) {
	fnt = try_add_font(fa, path, size());
	if (fnt)
		this->path = path;
	return fnt != NULL;
}

float Font::size() const noexcept {
	return ceil(pt * SDL::fnt_scale);
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
	arial.load(io.Fonts, "C:\\Windows\\Fonts\\arial.ttf");

	if (!(copper.load(io.Fonts, "C:\\Windows\\Fonts\\COPRGTB.TTF"))) {
		std::string localpath(std::string("C:\\Users\\") + get_username() + "\\AppData\\Local\\Microsoft\\Windows\\Fonts\\COPRGTB.TTF");
		copper.load(io.Fonts, localpath.c_str());
	}

	if (!(copper2.load(io.Fonts, "C:\\Windows\\Fonts\\COPRGTL.TTF"))) {
		std::string localpath(std::string("C:\\Users\\") + get_username() + "\\AppData\\Local\\Microsoft\\Windows\\Fonts\\COPRGTL.TTF");
		copper2.load(io.Fonts, localpath.c_str());
	}
#else
	#define FONT_DIR "/usr/share/fonts/truetype/"

	arial.load(io.Fonts, FONT_DIR "liberation/LiberationSans-Regular.ttf");
	copper.load(io.Fonts, FONT_DIR "abyssinica/AbyssinicaSIL-Regular.ttf");
	copper2.load(io.Fonts, FONT_DIR "ancient-scripts/Symbola_hint.ttf");
#endif
	return loaded();
}

}
