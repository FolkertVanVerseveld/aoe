#include "../../server.hpp"

#include <except.hpp>

#include "../../debug.hpp"

namespace aoe {

void NetPkg::cam_set(float x, float y, float w, float h) {
	ZoneScoped;
	PkgWriter out(*this, NetPkgType::cam_set);
	write("4i", pkgargs({(uint64_t)x, (uint64_t)y, (uint64_t)w, (uint64_t)h}), false);
}

NetCamSet NetPkg::get_cam_set() {
	ZoneScoped;
	read(NetPkgType::cam_set, "4I");

	NetCamSet s;
	s.x = i32(0);
	s.y = i32(1);
	s.w = i32(2);
	s.h = i32(3);

	return s;
}

}
