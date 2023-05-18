#include "../game.hpp"

#include <algorithm>

namespace aoe {

Particle::Particle(IdPoolRef ref, ParticleType type, float x, float y, unsigned subimage) : ref(ref), type(type), x(x), y(y), subimage(subimage) {}

bool Particle::imgtick(unsigned n) noexcept {
	bool more = false;
	unsigned end;

	switch (type) {
	case ParticleType::explode1:
	case ParticleType::explode2:
		end = 10u - 1u;
		more = subimage < end;
		subimage = std::clamp<float>(subimage + n, 0u, end);
		break;
	default:
		end = 8u - 1u;
		more = subimage < end;
		subimage = std::clamp<float>(subimage + n * 0.25, 0u, end);
		break;
	}

	return more;
}

}
