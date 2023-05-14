#pragma once

namespace aoe {

struct Resources final {
	int food, wood, gold, stone;

	Resources() : food(0), wood(0), gold(0), stone(0) {}
	Resources(int f, int w, int g, int s) : food(f), wood(w), gold(g), stone(s) {}

	/** Check if we have sufficient resources to deduct \a res from it. */
	constexpr bool can_afford(const Resources &res) const noexcept {
		return food >= res.food && wood >= res.wood && gold >= res.gold && stone >= res.stone;
	}

	Resources &operator+=(const Resources &rhs) {
		food += rhs.food;
		wood += rhs.wood;
		gold += rhs.gold;
		stone += rhs.stone;
		return *this;
	}

	Resources &operator-=(const Resources &rhs) {
		food -= rhs.food;
		wood -= rhs.wood;
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
