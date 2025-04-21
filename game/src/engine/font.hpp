#pragma once

struct ImFont;

namespace aoe {

class FontGuard final {
public:
	FontGuard(ImFont*);
	~FontGuard();
};

class FontCache final {
public:
	ImFont *fnt_arial, *fnt_copper, *fnt_copper2;

	FontCache();

	bool loaded() const noexcept;
	bool try_load();
};

extern FontCache fnt;

}
