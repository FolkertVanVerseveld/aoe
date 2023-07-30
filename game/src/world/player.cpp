#include "../game.hpp"
#include "../server.hpp"

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

Player::Player(const PlayerSetting &ps, size_t explored_max) : init(ps), res(ps.res), achievements(), explored_max(explored_max), entities(), alive(true), ai(ps.ai), ai_workers() {
	achievements.alive = alive;
}

PlayerView Player::view() const noexcept {
	PlayerView v(init);

	v.res = res;
	v.score = achievements.score;

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

void Player::tick_autotask(WorldView &wv) {
	for (IdPoolRef ref : entities) {
		Entity *e = wv.try_get(ref);

		if (!e || !e->autotask)
			continue;

		if (is_worker(e->type) && e->type != EntityType::villager && e->state == EntityState::alive) {
			// reasign worker
			EntityType tt = EntityType::desert_tree1;

			switch (e->type) {
			case EntityType::worker_gold: tt = EntityType::gold; break;
			case EntityType::worker_stone: tt = EntityType::stone; break;
			case EntityType::worker_berries: tt = EntityType::berries; break;
			}

			Entity *next_target = wv.try_get_alive(e->x, e->y, tt);
			if (next_target)
				e->task_attack(*next_target);
		}
	}
}

void Player::tick(WorldView &wv) {
	tick_autotask(wv);

	if (!ai)
		return;

	// TODO auto collect workers for any player, regardless if AI (e.g. for idle workers)
	ai_workers.clear();

	for (IdPoolRef ref : entities) {
		Entity *e = wv.try_get(ref);

		if (!e || !e->is_alive())
			continue;

		if (is_worker(e->type))
			ai_workers.emplace_back(ref);
	}

	// count worker tasks
	unsigned idle = 0, food = 0, wood = 0;

	for (IdPoolRef ref : ai_workers) {
		Entity &e = wv.at(ref);
		if (e.is_attacking()) {
			switch (e.type) {
			case EntityType::worker_berries:
				++food;
				break;
			case EntityType::worker_wood1:
			case EntityType::worker_wood2:
				++wood;
				break;
			}
		} else {
			++idle;
		}
	}

	// TODO assign 2/3 to food (round up), 1/3 to wood (round down)

	const float p_food = 2.0 / 3, p_wood = 1.0 - p_food;

	long need_food = (long)(ai_workers.size() * p_food);
	long need_wood = (long)(ai_workers.size() * p_wood);

	// use idle workers to balance tasks out
	if (idle) {
		// prefer food over wood, more food, more workers...
		if (need_food > 0) {
			unsigned more_food = (unsigned)ceil(need_food);

			for (IdPoolRef ref : ai_workers) {
				if (!more_food)
					return;

				Entity &e = wv.at(ref);
				if (e.is_attacking())
					continue;

				Entity *target = wv.try_get_alive(e.x, e.y, EntityType::berries);
				if (target)
					e.task_attack(*target);
			}
		}

		if (need_wood > 0) {
			unsigned more_wood = (unsigned)need_wood;

			for (IdPoolRef ref : ai_workers) {
				if (!more_wood)
					return;

				Entity &e = wv.at(ref);
				if (e.is_attacking())
					continue;

				Entity *target = wv.try_get_alive(e.x, e.y, EntityType::desert_tree1);
				if (target)
					e.task_attack(*target);
			}
		}
	}

	// TODO change worker task if distribution changes
	// TODO train more villagers
	// TODO check if we should resign
}

}
