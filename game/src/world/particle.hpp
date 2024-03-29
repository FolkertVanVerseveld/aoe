#pragma once

namespace aoe {

enum class ParticleType {
	moveto,
	explode1,
	explode2,
};

class Particle final {
public:
	IdPoolRef ref;
	ParticleType type;

	float x, y;

	float subimage;

	Particle(IdPoolRef ref, ParticleType type, float x, float y, unsigned subimage);

	/* Animate and return whether there's more images. Returning false means the animation has ended and the particle shall be destroyed. */
	bool imgtick(unsigned n) noexcept;

	friend bool operator<(const Particle &lhs, const Particle &rhs) noexcept {
		return lhs.ref < rhs.ref;
	}
};

}
