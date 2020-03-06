/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#pragma once

#include <cstdint>

namespace genie {

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

}
