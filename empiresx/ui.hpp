#pragma once

#include "drs.hpp"
#include "font.hpp"
#include "render.hpp"

#include <memory>

namespace genie {

bool str_to_ip(const std::string&, uint32_t&);

namespace ui {

class InteractableCallback {
public:
	virtual void interacted(unsigned index) = 0;
};

class Interactable {
protected:
	unsigned int_id;
	bool has_focus;
	bool is_pressed;
	InteractableCallback &cb;
public:
	Interactable(unsigned id, InteractableCallback &cb) : int_id(id), has_focus(false), is_pressed(false), cb(cb) {}

	virtual void focus(bool on) = 0;
	virtual void press(bool on) = 0;
};

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

	/** Rough estimation if it probably collides. */
	bool contains(const SDL_Point &pt) const {
		return contains(pt.x, pt.y);
	}

	bool contains(int x, int y) const;

	virtual bool collides(const SDL_Point &pt) const {
		return contains(pt);
	}

	virtual bool collides(int x, int y) const {
		SDL_Point pt{x, y};
		return collides(pt);
	}

	SDL_Rect bounds() const { return bnds; }

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
	field = 2
};

/** Raw border that only draws a simple rectangular box using background settings binary info. */
class Border : public DynamicUI {
public:
	SDL_Color cols[6];
	BorderType type;
	/** Optional background transparency (0=transparent, 255=opaque) */
	int shade;
	bool flip;

	Border(const SDL_Rect bnds[screen_modes], ConfigScreenMode mode, const Palette &pal, const BackgroundSettings &bkg, BorderType type, bool enhanced=false);

	void paint(SimpleRender &r) override;
	void paint(SimpleRender &r, const SDL_Color &bg);
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

class Button final : public Border, public ui::Interactable {
	/** Identifiable element (e.g. icon, text, ...) */
	std::unique_ptr<DynamicUI> decorator;
public:
	Button(unsigned id, InteractableCallback &cb, SimpleRender &r, const SDL_Rect bnds[screen_modes], ConfigScreenMode mode, const Palette &pal, const BackgroundSettings &bkg, DynamicUI *decorator, bool enhanced=false);
	Button(unsigned id, InteractableCallback &cb, SimpleRender &r, Font &f, const std::string &s, SDL_Color fg, SDL_Color bg, const SDL_Rect txt_bnds[screen_modes], const SDL_Rect bnds[screen_modes], const Palette &pal, const BackgroundSettings &bkg, ConfigScreenMode mode, HAlign=HAlign::center, VAlign=VAlign::middle, bool adjust_anchors=true, bool enhanced=false);

	void resize(ConfigScreenMode old_mode, ConfigScreenMode mode) override;
	void paint(SimpleRender &r) override;

	void focus(bool) override;
	void press(bool) override;
};

class InputField;

class InputCallback {
public:
	virtual bool input(unsigned id, InputField &field) = 0;
};

enum class InputType {
	text,
	number,
	port, // stricter than number
	ip,
};

class InputField final : public Border, public TextBuf {
	unsigned index;
	InputType type;
	InputCallback &cb;
	bool hasfocus;
public:
	bool error;

	InputField(unsigned id, InputCallback &cb, InputType type, const std::string &init, SimpleRender &r, Font &f, SDL_Color fg, const SDL_Rect bnds[screen_modes], ConfigScreenMode mode, const Palette &pal, const BackgroundSettings &bkg, bool enhanced=false);

	bool keydown(int ch);
	bool keyup(int ch);

	void focus(bool on);

	long long number() const;
	uint16_t port() const;
	bool ip(uint32_t &addr) const;
	const std::string &str() const;
	const std::string &text() const;

	void paint(SimpleRender &r) override;
};

}
}
