/* Copyright 2020 Folkert van Verseveld. All rights reserved */

#include <cmath>
#include <cstdint>

// not guaranteed to be defined
#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

namespace genie {

static constexpr bool ispow2(uint64_t v) noexcept {
	return v && (v & (v - 1)) == 0;
}

static constexpr uint64_t nextpow2(uint64_t v) noexcept {
	--v;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v |= v >> 32;
	return ++v;
}

static constexpr uint64_t prevpow2(uint64_t v) noexcept {
	while (v & v - 1)
		v &= v - 1;

	return v;
}

template<typename T> constexpr T deg2rad(T v) noexcept { return v * M_PI / 180.0; }
template<typename T> constexpr T rad2deg(T v) noexcept { return v * 180.0 / M_PI; }

/** Determine whether \a v is positive, negative or zero. */
template<typename T> int signum(T v) noexcept { return v > 0 ? 1 : v < 0 ? -1 : 0; }

/** Ensure the \a v lies in range [min, max] */
template<typename T> constexpr T clamp(T min, T v, T max) noexcept { return v < min ? min : v < max ? v : max; }

}
