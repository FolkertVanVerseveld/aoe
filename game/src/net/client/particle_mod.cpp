#include "../../server.hpp"

#include "../../engine.hpp"

namespace aoe {

void Client::particlemod(NetPkg &pkg) {
	Particle p = pkg.get_particle();
	g.particle_spawn(p);
}

}
