#define SDL_MAIN_HANDLED 1

#include "../src/engine.hpp"

#include "util.hpp"

#include <gtest/gtest.h>

namespace aoe {

TEST_F(NoHeadlessFixture, engineCreateDelete) {
	Engine eng;
	(void)eng;
}

TEST_F(NoHeadlessFixture, engineCreateTwice) {
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
