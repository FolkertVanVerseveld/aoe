/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#pragma once

#include <cstdint>

namespace genie {

#ifndef M_PI
#    define M_PI 3.14159265358979323846
#endif

static constexpr int tw = 64, th = 32;

static constexpr inline bool ispow2(uint64_t v) noexcept {
	return v && !(v & (v - 1));
}

static constexpr inline uint64_t nextpow2(uint64_t v) noexcept {
	--v;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v |= v >> 32;
	return ++v;
}

static constexpr inline uint64_t makepow2(uint64_t v) noexcept {
	return ispow2(v) ? v : nextpow2(v);
}

template<typename T>
static constexpr void tile_to_scr(T &x, T &y, T tx, T ty) {
	y = (tx - ty) * th / 2;
	x = (tx + ty) * tw / 2;
}

}
