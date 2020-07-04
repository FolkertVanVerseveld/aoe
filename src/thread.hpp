/* Copyright 2019-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include <thread>

namespace genie {

extern std::thread::id t_main;

static bool is_t_main() {
	return t_main == std::this_thread::get_id();
}

}
