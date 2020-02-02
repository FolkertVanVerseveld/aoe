#include "ui.hpp"

namespace genie {
namespace ui {

Border::Border(const SDL_Rect &bnds, const Palette &pal, const BackgroundSettings &bkg) : DynamicUI(bnds) {
	for (unsigned i = 0; i < 6; ++i)
		cols[i] = pal.tbl[bkg.bevel[i]];
}

void Border::paint(SimpleRender &r) {
	r.border(bnds, cols);
}

}
}
