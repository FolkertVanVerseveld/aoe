#pragma once

#include "drs.hpp"
#include "render.hpp"

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
	virtual void paint(SimpleRender &r) = 0;
};

class DynamicUI : public UI {
protected:
	SDL_Rect lgy_bnds[screen_modes];

	DynamicUI(const SDL_Rect bnds[screen_modes], ConfigScreenMode mode);
public:
	virtual void resize(const SDL_Rect &bnds) {
		this->bnds = bnds;
	}
};

enum class BorderType {
	background = 0,
	button = 1,
};

class Border final : public DynamicUI {
public:
	SDL_Color cols[6];
	BorderType type;
	int shade;

	Border(const SDL_Rect bnds[screen_modes], ConfigScreenMode mode, const Palette &pal, const BackgroundSettings &bkg, BorderType type);

	void paint(SimpleRender &r) override;
};

}
}
