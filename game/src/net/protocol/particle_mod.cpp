#include "../../server.hpp"

#include <cassert>
#include <except.hpp>

#include <cmath>

#include "../../debug.hpp"

namespace aoe {

void NetPkg::particle_spawn(const Particle &p) {
	PkgWriter out(*this, NetPkgType::particle_mod);

	clear();
	writef("2IHF", p.ref.first, p.ref.second, p.type, p.subimage);
	writef("2F2b", p.x, p.y,
		(int8_t)(INT8_MAX * fmodf(p.x, 1)), (int8_t)(INT8_MAX * fmodf(p.y, 1)));
}

Particle NetPkg::get_particle() {
	ntoh();

	if ((NetPkgType)hdr.type != NetPkgType::particle_mod)
		throw std::runtime_error("not a particle control packet");

	args.clear();
	unsigned pos = 0;

	pos += read("2I2H2I2b", args, pos);

	IdPoolRef ref;
	ref.first = u32(0); ref.second = u32(1);
	unsigned type = u16(2), subimage = u16(3);
	float x = u32(4), y = u32(5);
	int8_t sx = i8(6), sy = i8(7);

	if (sx || sy) {
		x += sx / (float)INT8_MAX;
		y += sy / (float)INT8_MAX;
	}

	return Particle(ref, (ParticleType)type, x, y, subimage);
}

}
