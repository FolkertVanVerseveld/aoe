#pragma once

#include "drs.hpp"
#include "font.hpp"
#include "render.hpp"

#include <memory>

namespace genie {
namespace ui {

class UI {
protected:
	SDL_Rect bnds;

	UI() {
		bnds.x = bnds.y = 0;
		bnds.w = bnds.h = 1;
	}

	UI(const SDL_Rect &bnds) : bnds(bnds) {}
public:
	virtual ~UI() {}

	virtual void paint(SimpleRender &r) = 0;
};

/** Special user interface element that supports dynamic scaling. */
class DynamicUI : public UI {
protected:
	/** Holds all hardcoded and dynamically dimensions for each screen mode. */
	SDL_Rect scr_bnds[screen_modes];
	bool enhanced;

	DynamicUI(const SDL_Rect bnds[screen_modes], ConfigScreenMode mode, bool enhanced=false);
public:
	virtual ~DynamicUI() {}

	virtual void resize(ConfigScreenMode old_mode, ConfigScreenMode mode);
};

enum class BorderType {
	background = 0,
	button = 1,
};

/** Raw border that only draws a simple rectangular box using background settings binary info. */
class Border final : public DynamicUI {
public:
	SDL_Color cols[6];
	BorderType type;
	/** Optional background transparency (0=transparent, 255=opaque) */
	int shade;

	Border(const SDL_Rect bnds[screen_modes], ConfigScreenMode mode, const Palette &pal, const BackgroundSettings &bkg, BorderType type, bool enhanced=false);

	void paint(SimpleRender &r) override;
};

/** User interface horizontal anchor point. */
enum class HAlign {
	left,
	center,
	right,
};

/** User interface vertical anchor point. */
enum class VAlign {
	top,
	middle,
	bottom
};

/** Raw font rendered text. */
class Label final : public DynamicUI {
public:
	SDL_Color fg, bg;
	Text txt_fg, txt_bg; // XXX OpenGL only needs one?
	HAlign halign;
	VAlign valign;

	Label(SimpleRender &r, Font &f, const std::string &s, SDL_Color fg, SDL_Color bg, const SDL_Rect bnds[screen_modes], ConfigScreenMode mode, HAlign=HAlign::left, VAlign=VAlign::top, bool adjust_anchors=false, bool enhanced=false);
	Label(SimpleRender &r, Font &f, const std::string &s, const SDL_Rect bnds[screen_modes], ConfigScreenMode mode, const Palette &pal, const BackgroundSettings &bkg, HAlign=HAlign::left, VAlign=VAlign::top, bool adjust_anchors=false, bool enhanced=false);

	void resize(ConfigScreenMode old_mode, ConfigScreenMode mode) override;
	void paint(SimpleRender &r) override;
};

/* High level UI elements */

class Button final : public ui::DynamicUI {
	ui::Border border;
	/** Identifiable element (e.g. icon, text, ...) */
	std::unique_ptr<DynamicUI> decorator;
public:
	Button(SimpleRender &r, const SDL_Rect bnds[screen_modes], ConfigScreenMode mode, const Palette &pal, const BackgroundSettings &bkg, DynamicUI *decorator, bool enhanced=false);
	Button(SimpleRender &r, Font &f, const std::string &s, SDL_Color fg, SDL_Color bg, const SDL_Rect txt_bnds[screen_modes], const SDL_Rect bnds[screen_modes], const Palette &pal, const BackgroundSettings &bkg, ConfigScreenMode mode, HAlign=HAlign::center, VAlign=VAlign::middle, bool adjust_anchors=true, bool enhanced=false);

	void resize(ConfigScreenMode old_mode, ConfigScreenMode mode) override;
	void paint(SimpleRender &r) override;
};

}
}
