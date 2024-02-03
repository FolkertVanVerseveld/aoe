#define SDL_MAIN_HANDLED 1

#include "../src/engine.hpp"

#include <gtest/gtest.h>

namespace aoe {

extern void net_runall();
extern void server_runall();

TEST(Engine, CreateDelete) {
	Engine eng;
	(void)eng;
}

TEST(Engine, CreateTwice) {
	Engine eng1;
	try {
		Engine eng2;
		FAIL() << "created twice";
	} catch (std::runtime_error&) {}
}

static void runall() {
	net_runall();
	server_runall();
}

}

int main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);
	//aoe::runall();
	return RUN_ALL_TESTS();
}
