#pragma once

#include <cassert>
#include <vector>

namespace aoe {

template<typename T> class Vector2dView final {
	std::vector<T> &v;
	const size_t w;
public:
	Vector2dView(std::vector<T> &v, size_t w) : v(v), w(w) {
		// v must be divisible by w
		assert((v.size() % w) == 0);
	}

	T &at(size_t x, size_t y) {
		return v.at(idx(x, y));
	}

	constexpr size_t idx(size_t x, size_t y) const noexcept {
		return y * w + x;
	}
};

}
