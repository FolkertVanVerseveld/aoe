#include "../../server.hpp"

#include <except.hpp>

#include "../../debug.hpp"

namespace aoe {

void NetPkg::cam_set(float x, float y, float w, float h) {
	ZoneScoped;
	PkgWriter out(*this, NetPkgType::cam_set);
	clear();
	writef("4F", x, y, w, h);
}

NetCamSet NetPkg::get_cam_set() {
	ZoneScoped;
	read(NetPkgType::cam_set, "4F");

	NetCamSet s;
	s.x = F32(0);
	s.y = F32(1);
	s.w = F32(2);
	s.h = F32(3);

	return s;
}

}
