#define SDL_MAIN_HANDLED 1

#include "../src/engine.hpp"

#include <gtest/gtest.h>

namespace aoe {

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

}

int main(int argc, char **argv)
{
	testing::InitGoogleTest(&argc, argv);
	//aoe::runall();
	return RUN_ALL_TESTS();
}
