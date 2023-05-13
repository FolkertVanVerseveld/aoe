#include "../game.hpp"

#include <algorithm>

namespace aoe {

Particle::Particle(IdPoolRef ref, ParticleType type, float x, float y, unsigned subimage) : ref(ref), type(type), x(x), y(y), subimage(subimage) {}

bool Particle::imgtick(unsigned n) noexcept {
	unsigned end = 8u - 1u;
	subimage = std::clamp<float>(subimage + n * 0.25, 0u, end);
	return subimage < end;
}

}
