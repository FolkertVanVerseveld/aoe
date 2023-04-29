#include "../../engine.hpp"

namespace aoe {

EngineView::EngineView() : lk(m_eng) {
	if (!eng)
		throw std::runtime_error("engine not running");
}

void EngineView::play_sfx(SfxId id, int loops) {
	eng->sfx.play_sfx(id, loops);
}

}
