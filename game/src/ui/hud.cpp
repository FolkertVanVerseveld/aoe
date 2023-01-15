#include "../ui.hpp"

#include "../engine.hpp"

#include <tracy/Tracy.hpp>

namespace aoe {

namespace ui {

void UICache::user_interact_entities() {
	ZoneScoped;

	if (!e->keyctl.is_tapped(GameKey::kill_entity) && !selected.empty())
		return;

	// find first entity in selection
	for (IdPoolRef ref : selected) {
		if (e->gv.buildings.try_invalidate(ref)) {
			puts("kill");
			e->sfx.play_sfx(SfxId::bld_die_random);
			break;
		}
	}
}

}

}
