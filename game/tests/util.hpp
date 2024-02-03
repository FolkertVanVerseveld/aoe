#pragma once

#include <cstddef>
#include <cstdarg>

#include <gtest/gtest.h>

#define myFAIL(fmt, ...) fail(__FILE__, __LINE__, __func__, fmt, ## __VA_ARGS__)
#define myVFAIL(fmt, args) vfail(__FILE__, __LINE__, __func__, fmt, args)

namespace aoe {

class NoUnixOrTracyFixture : public ::testing::Test {
protected:
	void SetUp() override {
#if _WIN32 == 0 || TRACY_ENABLE
		GTEST_SKIP() << "net is already initialised by C runtime";
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

void fail(const char *file, size_t lno, const char *func, const char *fmt, ...);
void vfail(const char *file, size_t lno, const char *func, const char *fmt, va_list args);

}
