#include "../../server.hpp"

#include "../../engine.hpp"

namespace aoe {

void Client::resource_ctl(NetPkg &pkg) {
	Resources res = pkg.get_resources();
	g.set_player_resources(playerindex, res);
}

}
