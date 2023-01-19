#include "../game.hpp"

namespace aoe {

int64_t PlayerAchievements::recompute(size_t max_military, size_t max_villagers, unsigned max_tech) noexcept {
	// https://ageofempires.fandom.com/wiki/Score
	score = 0;

	military_score = 0;

	military_score = std::max<int64_t>(0, kills / 2 - losses) + razings;

	// TODO

	return score;
}

Player::Player(const PlayerSetting &ps, size_t explored_max) : init(ps), res(ps.res), achievements(), explored_max(explored_max), entities(), alive(true) {}

PlayerView Player::view() const noexcept {
	PlayerView v(init);

	v.res = res;
	v.score = achievements.score;
	v.alive = alive;

	return v;
}

}
