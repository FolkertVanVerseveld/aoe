#include "../../server.hpp"

namespace aoe {

ClientView::ClientView() : me(invalid_ref), scn(), gameover(false) {}

bool ClientView::try_read(Client &c) {
	std::unique_lock lk(c.m, std::defer_lock);

	if (!lk.try_lock())
		return false;

	// only copy what has been changed
	if (c.modflags & (unsigned)ClientModFlags::scn)
		scn = c.scn;
	if (c.modflags & (unsigned)ClientModFlags::terrain)
		me = c.me;

	gameover = c.gameover;
	victory = c.victory;

	c.modflags = 0;

	return true;
}

}
