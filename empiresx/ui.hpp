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
	DynamicUI() : UI() {}
	DynamicUI(const SDL_Rect &bnds) : UI(bnds) {}
public:
	virtual void resize(const SDL_Rect &bnds) {
		this->bnds = bnds;
	}
};

class Border final : public DynamicUI {
public:
	SDL_Color cols[6];

	Border(const SDL_Rect &bnds, const Palette &pal, const Background &bkg);

	void paint(SimpleRender &r) override;
};

}
}
