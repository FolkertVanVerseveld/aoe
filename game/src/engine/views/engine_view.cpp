#include "../../engine.hpp"

namespace aoe {

EngineView::EngineView() : lk(m_eng) {
	if (!eng) {
		lk.unlock();
		throw std::runtime_error("engine not running");
	}
}

void EngineView::trigger_async_flags(EngineAsyncTask t) {
	eng->trigger_async_flags(t);
}

void EngineView::goto_menu(MenuState ms) {
	eng->next_menu_state = ms;
}

void EngineView::play_sfx(SfxId id, int loops) {
	eng->sfx.play_sfx(id, loops);
}

}
