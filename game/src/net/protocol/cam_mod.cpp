#include "../../server.hpp"

#include <except.hpp>

#include "../../debug.hpp"

namespace aoe {

void NetPkg::cam_set(float x, float y, float w, float h) {
	ZoneScoped;
	PkgWriter out(*this, NetPkgType::cam_set);
	write("4i", pkgargs{ x, y, w, h }, false);
}

NetCamSet NetPkg::get_cam_set() {
	ZoneScoped;
	ntoh();

	if ((NetPkgType)hdr.type != NetPkgType::cam_set)
		throw std::runtime_error("not a camera set packet");

	NetCamSet s;
	args.clear();
	read("4I", args);

	s.x = i32(0);
	s.y = i32(1);
	s.w = i32(2);
	s.h = i32(3);

	return s;
}

}
