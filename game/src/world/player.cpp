#include "../game.hpp"

#include <minmax.hpp>

namespace aoe {

int64_t PlayerAchievements::recompute() noexcept {
	// https://ageofempires.fandom.com/wiki/Score
	score = 0;

	military_score = (uint32_t)(std::max<int64_t>(0, ((int64_t)kills - losses) / 2 + razings));

	// NOTE economy score might be negative when tribute is negative
	economy_score = gold / 100 + tribute / 60 + villager_count;

	religion_score = conversions * 2 + temples * 3 + ruins * 10 + artifacts * 10;

	technology_score = technologies * 2;

	// summarize each category
	score += military_score;
	score += economy_score;
	score += religion_score;
	score += technology_score;
	score += 100 * wonders;

	if (alive)
		score += 100;

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

PlayerAchievements Player::get_score() noexcept {
	achievements.alive = alive;
	achievements.recompute();
	return achievements;
}

void Player::lost_entity(IdPoolRef ref) {
	if (!entities.erase(ref))
		return;

	++achievements.losses;
}

void Player::killed_unit() {
	++achievements.kills;
}

void Player::killed_building() {
	++achievements.razings;
}

}
