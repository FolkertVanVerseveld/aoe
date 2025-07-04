#pragma once

#include <string>

struct ImFont;
struct ImFontAtlas;

namespace aoe {

class Font final {
public:
	ImFont *fnt;
	std::string path;
	float pt;

	Font(float pt);

	bool load(ImFontAtlas *fa, const char *path);
	float size() const noexcept;

	operator bool() const noexcept { return fnt != NULL; }
};

class FontGuard final {
public:
	FontGuard(const Font&);
	~FontGuard();
};

class FontCache final {
public:
	Font arial, copper, copper2;

	FontCache();

	bool loaded() const noexcept;
	bool try_load();
};

extern FontCache fnt;

}
