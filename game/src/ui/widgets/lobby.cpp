#include "../../engine.hpp"

#include "../../ui.hpp"

namespace aoe {

using namespace ui;

void UICache::show_mph_tbl(ui::Frame &f) {
	Table t;

	unsigned idx = 0;
	ScenarioSettings &scn = e->cv.scn;

	auto it = scn.owners.find(e->cv.me);
	if (it != scn.owners.end())
		idx = it->second;

	if (t.begin("PlayerTable", 3)) {
		t.row(-1, {"Name", "Civ", "Team"});

		unsigned del = scn.players.size();
		unsigned from = 0, to = 0;

		for (unsigned i = 1; i < scn.players.size(); ++i) {
			Row r(3, i);
			PlayerSetting &p = scn.players[i];

			if (e->multiplayer_ready) {
				f.str(p.name);
				r.next();

				f.str(civs.at(p.civ));
				r.next();

				f.str(std::to_string(p.team));
				r.next();
				continue;
			}

			if (i == idx) {
				if (f.btn("X"))
					e->client->claim_player(0);

				f.sl();

				if (r.text("##0", p.name, ImGuiInputTextFlags_EnterReturnsTrue))
					e->client->send_set_player_name(i, p.name);
			} else {
				if (f.btn("Claim"))
					e->client->claim_player(i);

				f.sl();

				if (!p.ai && e->server.get() != nullptr && f.btn("Set CPU"))
					e->client->claim_cpu(i);

				r.next();
			}

			if (f.combo("##1", p.civ, civs))
				e->client->send_set_player_civ(i, p.civ);
			r.next();

			p.team = std::max(1u, p.team);
			if (f.scalar("##2", p.team, 1))
				e->client->send_set_player_team(i, p.team);
			r.next();
		}
	}
}

}
