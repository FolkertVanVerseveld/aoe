#pragma once

#include <cstddef>
#include <cstdarg>

#include <gtest/gtest.h>

namespace aoe {

class NoUnixOrTracyFixture : public ::testing::Test {
protected:
	void SetUp() override {
#if _WIN32 == 0 || TRACY_ENABLE
		GTEST_SKIP() << "net is already initialised by C runtime";
#endif
	}
};

class NoUnixFixture : public ::testing::Test {
protected:
	void SetUp() override {
#if _WIN32 == 0
		GTEST_SKIP() << "WIN32 only test";
#endif
	}
};

class NoHeadlessFixture : public ::testing::Test {
protected:
	void SetUp() override {
#if BUILD_TESTS_HEADLESS
		GTEST_SKIP() << "skipping UI engine tests as we are running headless";
#endif
	}
};

static void dump_errors(std::vector<std::string> &bt) {
	if (bt.empty())
		return;

	for (size_t i = 0; i < bt.size() - 1; ++i)
		ADD_FAILURE() << bt[i];

	FAIL() << bt.back();
}

}
