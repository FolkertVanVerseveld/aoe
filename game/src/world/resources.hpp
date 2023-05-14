#pragma once

namespace aoe {

struct Resources final {
	int wood, food, gold, stone;

	Resources() : wood(0), food(0), gold(0), stone(0) {}
	Resources(int w, int f, int g, int s) : wood(w), food(f), gold(g), stone(s) {}

	/** Check if we have sufficient resources to deduct \a res from it. */
	constexpr bool can_afford(const Resources &res) const noexcept {
		return wood >= res.wood && food >= res.food && gold >= res.gold && stone >= res.stone;
	}

	Resources &operator+=(const Resources &rhs) {
		wood += rhs.wood;
		food += rhs.food;
		gold += rhs.gold;
		stone += rhs.stone;
		return *this;
	}

	Resources &operator-=(const Resources &rhs) {
		wood -= rhs.wood;
		food -= rhs.food;
		gold -= rhs.gold;
		stone -= rhs.stone;
		return *this;
	}

	friend Resources operator+(Resources lhs, const Resources &rhs) {
		lhs += rhs;
		return lhs;
	}

	friend Resources operator-(Resources lhs, const Resources &rhs) {
		lhs -= rhs;
		return lhs;
	}
};

}
