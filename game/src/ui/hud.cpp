#include "../ui.hpp"

#include "../engine.hpp"

#include <tracy/Tracy.hpp>

namespace aoe {

namespace ui {

void UICache::idle() {
	ZoneScoped;

	auto &killed = e->gv.entities_killed;

	if (killed.empty())
		return;

	for (const EntityView &ev : killed) {
		// TODO filter if out of camera range
		if (is_building(ev.type))
			e->sfx.play_sfx(SfxId::bld_die_random);
	}

	killed.clear();

	for (unsigned pos : e->gv.players_died) {
		PlayerView &pv = e->gv.players[pos];
		e->add_chat_text(pv.init.name + " resigned");
		e->sfx.play_sfx(SfxId::player_resign);
	}
}

void UICache::user_interact_entities() {
	ZoneScoped;

#if 1
	if (e->keyctl.is_tapped(GameKey::kill_entity) && !selected.empty()) {
		e->client->entity_kill(*selected.begin());
		return;
	}
#else
	if (!e->keyctl.is_tapped(GameKey::kill_entity) && !selected.empty())
		return;

	// find first entity in selection
	for (IdPoolRef ref : selected) {
		if (e->gv.entities.try_invalidate(ref)) {
			e->sfx.play_sfx(SfxId::bld_die_random);
			break;
		}
	}
#endif
}

}

}
