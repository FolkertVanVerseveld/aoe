#include "../../server.hpp"

namespace aoe {

ClientView::ClientView() : me(invalid_ref), scn(), gameover(false), victory(false), playerindex(0) {}

bool ClientView::try_read(IClient &c) {
	std::unique_lock lk(c.m, std::defer_lock);

	if (!lk.try_lock())
		return false;

	// only copy what has been changed
	if (c.modflags & (unsigned)ClientModFlags::scn)
		scn = c.scn;
	if (c.modflags & (unsigned)ClientModFlags::ref)
		me = c.me;

	gameover = c.gameover;
	victory = c.victory;
	playerindex = c.playerindex;

	c.modflags = 0;

	return true;
}

}
