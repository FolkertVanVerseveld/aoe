#include "../../server.hpp"

#include "../../engine.hpp"

namespace aoe {

void Client::cam_ctl(NetPkg &pkg) {
	NetCamSet cam(pkg.get_cam_set());

	EngineView ev;
	ev.cam_set(cam.x, cam.y);
}

}