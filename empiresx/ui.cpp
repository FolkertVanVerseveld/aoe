#include "ui.hpp"

namespace genie {
namespace ui {

DynamicUI::DynamicUI(const SDL_Rect bnds[screen_modes], ConfigScreenMode mode) : UI(bnds[(unsigned)mode]) {
	for (unsigned i = 0; i < screen_modes; ++i)
		lgy_bnds[i] = bnds[i];
}

Border::Border(const SDL_Rect bnds[screen_modes], ConfigScreenMode mode, const Palette &pal, const BackgroundSettings &bkg, BorderType type) : DynamicUI(bnds, mode), type(type) {
	for (unsigned i = 0; i < 6; ++i)
		cols[i] = pal.tbl[bkg.bevel[i]];

	switch (type) {
	case BorderType::background:
		shade = 0;
		break;
	case BorderType::button:
		shade = bkg.shade;
		break;
	}
}

void Border::paint(SimpleRender &r) {
	r.border(bnds, cols, shade);
}

}
}
