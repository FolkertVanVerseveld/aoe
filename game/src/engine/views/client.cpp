#include "../../server.hpp"

namespace aoe {

ClientView::ClientView() : me(invalid_ref), scn() {}

bool ClientView::try_read(Client &c) {
	std::unique_lock lk(c.m, std::defer_lock);

	if (!lk.try_lock())
		return false;

	// TODO only copy what has been changed
	scn = c.scn;
	me = c.me;

	return true;
}

}
