#include "game_settings.hpp"

#include "../../engine/assets.hpp"

#include <random>

namespace aoe {

bool sp_randomize_teams = true;

PlayerSetting::PlayerSetting()
	: name(), color(0), civ(0), team(1), res(), active(false), type(PlayerType::human) {}

PlayerSetting::PlayerSetting(const std::string &name, int civ, unsigned team, Resources res)
	: name(name), color(0), civ(civ), team(team), res(res)
	, active(false), type(PlayerType::human) {}

void sp_game_settings_randomize() {
	unsigned civs = old_lang.civ_names.size();

	if (civs < 1)
		return;

	std::random_device rd;
	std::mt19937 gen(rd());

	std::uniform_int_distribution<> rngCiv(0, civs - 1);
	std::uniform_int_distribution<> rngTeam(first_team_idx, sp_player_count - first_team_idx);

	for (unsigned i = first_player_idx, j = 1; i < sp_player_count; ++i, ++j) {
		PlayerSetting &ps = sp_players[i];

		ps.civ = rngCiv(gen);

		auto &names = old_lang.civs.at(old_lang.civ_names[ps.civ]);

		std::uniform_int_distribution<> rngName(0, names.size() - 1);
		ps.name = names[rngName(gen)];

		ps.team = sp_randomize_teams ? rngTeam(gen) : j;
	}
}

}
