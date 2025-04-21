#include "game_settings.hpp"

#include "../../engine/assets.hpp"

#include <random>

namespace aoe {

void sp_game_settings_randomize() {
	unsigned civs = old_lang.civ_names.size();

	if (civs < 1)
		return;

	std::random_device rd;
	std::mt19937 gen(rd());

	std::uniform_int_distribution<> rngCiv(0, civs - 1);
	std::uniform_int_distribution<> rngTeam(first_team_idx, sp_player_count - first_team_idx);

	for (unsigned i = first_player_idx; i < sp_player_count; ++i) {
		PlayerSetting &ps = sp_players[i];

		ps.civ = rngCiv(gen);

		auto &names = old_lang.civs.at(old_lang.civ_names[ps.civ]);

		std::uniform_int_distribution<> rngName(0, names.size() - 1);
		ps.name = names[rngName(gen)];
		ps.team = rngTeam(gen);
	}
}

}
