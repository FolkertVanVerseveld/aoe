#include "../../engine.hpp"

#include "../../ui.hpp"

#include "../../world/game/game_settings.hpp"
#include "../../engine/assets.hpp"

namespace aoe {

namespace ui {

static const ImVec4 player_colors[8] = {
	{0.0f, 0.0f, 192 / 255.0f, 1.0f},
	{200 / 255.0f, 0.0f, 0.0f, 1.0f},
	{234 / 255.0f, 234 / 255.0f, 0.0f, 1.0f},
	{140 / 255.0f, 90 / 255.0f, 33 / 255.0f, 1.0f},
	{255 / 255.0f, 128 / 255.0f, 0.0f, 1.0f},
	{0.0f, 128 / 255.0f, 0.0f, 1.0f},
	{128 / 255.0f, 128 / 255.0f, 128 / 255.0f, 1.0f},
	{64 / 255.0f, 128 / 255.0f, 128 / 255.0f, 1.0f},
};

void show_player_game_table(ui::Frame &f) {
	Table t;

	unsigned idx = 0;

	if (!t.begin("PlayerTable", 3))
		return;

	t.row(-1, { "Name", "Civ", "Team" });

	for (unsigned i = 1, n = std::min<unsigned>(sp_players.size(), sp_player_ui_count) + 1; i < n; ++i) {
		Row r(3, i);
		PlayerSetting &p = sp_players[i];

		r.text("##0", p.name, ImGuiInputTextFlags_EnterReturnsTrue);
		f.combo("##1", p.civ, old_lang.civ_names);
		r.next();

		p.team = std::clamp(p.team, 1u, max_legacy_players - 1);
		f.scalar("##2", p.team, 1);
		r.next();
	}
}

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

			if (i == idx) { // show unclaim and player name if selected
				if (f.btn("X"))
					e->client->claim_player(0);

				f.sl();

				if (r.text("##0", p.name, ImGuiInputTextFlags_EnterReturnsTrue))
					e->client->send_set_player_name(i, p.name);
			} else { // allow client to select this one
				if (f.btn("Claim"))
					e->client->claim_player(i);

				r.next();
			}

			if (f.combo("##1", p.civ, civs))
				e->client->send_set_player_civ(i, p.civ);
			r.next();

			p.team = std::max(1u, p.team);

			unsigned pos = (p.team + 8 - 1) % 8;
			ImGui::PushStyleColor(ImGuiCol_Text, player_colors[pos]);
			if (f.scalar("##2", p.team, 1))
				e->client->send_set_player_team(i, p.team);
			ImGui::PopStyleColor();

			r.next();
		}
	}
}

}

}
