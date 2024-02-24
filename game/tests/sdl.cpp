#include "../src/engine/sdl.hpp"
#include "../src/time.h"

#include <cstddef>
#include <cstring>

#include <cstdio>

#include <memory>
#include <stdexcept>

#include <gtest/gtest.h>

namespace aoe {

static bool skip_chk_stall = false;

class SDLFixture : public ::testing::Test {
protected:
	void SetUp() override {
#if BUILD_TESTS_HEADLESS
		GTEST_SKIP() << "no ui tests as we are testing in headless environment";
#else
#define EXP "test"
		const char *got;

		SDL_SetError(EXP);
		if (strcmp(got = SDL_GetError(), EXP))
			GTEST_SKIP() << "expected \"" << EXP << "\", but got \"" << got << "\"";

		SDL_ClearError();
#undef EXP
#define EXP ""
		if (strcmp(got = SDL_GetError(), EXP))
			GTEST_SKIP() << "expected \"" << EXP << "\", but got \"" << got << "\"";
#undef EXP
#endif
	}
};

class SDL_guard final {
public:
	Uint32 flags;

	SDL_guard(Uint32 flags = SDL_INIT_VIDEO) : flags(flags) {
		if (SDL_Init(flags))
			throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
	}

	~SDL_guard() {
		SDL_Quit();
	}
};

static void check_error(const char *func, size_t lno) {
	const char *err = SDL_GetError();
	if (err && err[strspn(err, " \t")])
		FAIL() << func << ":" << lno << ": " << err;

	SDL_ClearError();
}

static bool is_error() {
	const char *err = SDL_GetError();
	return err && err[strspn(err, " \t")];
}

#define chkerr() check_error(__func__, __LINE__)

TEST_F(SDLFixture, QuitBeforeInit) {
	SDL_Quit();
	chkerr();
}

TEST_F(SDLFixture, Init) {
	SDL_guard g(0);
	SDL_GetNumVideoDisplays();

	if (!is_error())
		FAIL() << "main not ready, but init works";
}


TEST_F(SDLFixture, InitCheckStall) {
	timespec before, after;

	clock_gettime(CLOCK_MONOTONIC, &before);
	{
		SDL_guard g(SDL_INIT_VIDEO);
		chkerr();
	}
	clock_gettime(CLOCK_MONOTONIC, &after);

	double dt_ref = elapsed_time(&before, &after);

	clock_gettime(CLOCK_MONOTONIC, &before);
	{
		SDL_guard g(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER);
		chkerr();
	}
	clock_gettime(CLOCK_MONOTONIC, &after);

	double dt = elapsed_time(&before, &after);
	double slowdown = dt / dt_ref;

	if (dt > 10 && slowdown > 3) {
		FAIL() << "dt_ref=" << std::fixed << std::setprecision(2) << dt_ref << " sec, dt=" << dt << " sec (slowdown of " << slowdown << ")" << std::defaultfloat;
		//fprintf(stderr, "%s: compiled for SDL %d.%d.%d\n", __func__, SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
	}
}

TEST_F(SDLFixture, QueryDisplays) {
	SDL_guard sdl;

	int count = SDL_GetNumVideoDisplays();
	chkerr();
	if (count < 1) {
		FAIL() << "expected positive count, but got " << count;
		return;
	}

	printf("%s: detected %d %s\n", __func__, count, count == 1 ? "display" : "displays");

	for (int i = 0; i < count; ++i) {
		SDL_Rect bnds;

		if (SDL_GetDisplayBounds(i, &bnds)) {
			ADD_FAILURE() << "unknown bounds for display " << i << ": " << SDL_GetError();
			SDL_ClearError();
			continue;
		}

		printf("display %d: %4dx%4d at %4d,%4d\n", i, bnds.w, bnds.h, bnds.x, bnds.y);
	}

	for (int i = 0; i < count; ++i) {
		SDL_Rect bnds, bnds2;

		SDL_GetDisplayBounds(i, &bnds);
		SDL_GetDisplayUsableBounds(i, &bnds2);

		putchar('\n');
		printf("display %d:\n", i);
		printf("  resolution : %4dx%4d at %4d,%4d\n", bnds.w, bnds.h, bnds.x, bnds.y);
		printf("  usable area: %4dx%4d at %4d,%4d\n", bnds2.w, bnds2.h, bnds2.x, bnds2.y);

		double aspect = (double)bnds.w / bnds.h;

		// try to determine exact ratio
		long long llw = (long long)bnds.w, llh = (long long)bnds.h;
		char *fmt = "unknown format";

		// check both ways as we may have rounding errors.
		// i didn't verify this formula, so there might be situations were it isn't exact
		if (llw / 4 * 3 == llh && llh / 3 * 4 == llw)
			fmt = "4:3";
		else if (llw / 16 * 9 == llh && llh / 9 * 16 == llw)
			fmt = "16:9";
		else if (llw / 21 * 9 == llh && llh / 9 * 21 == llw)
			fmt = "21:9";

		printf("  aspect ratio: %s (%.2f)\n", fmt, aspect);

		int modes = SDL_GetNumDisplayModes(i);
		if (modes < 0) {
			fprintf(stderr, "%s: no modes found for display %d\n", __func__, i);
			continue;
		}

		printf("  display modes: %d\n", modes);

#if 0
		for (int j = 0; j < modes; ++j) {
			SDL_DisplayMode m;

			if (SDL_GetDisplayMode(i, j, &m)) {
				fprintf(stderr, "%s: bad mode %d for display %d: %s\n", __func__, j, i, SDL_GetError());
				continue;
			}

			printf("\n  mode %d:\n", j);
			printf("    resolution: %4dx%4d\n", m.w, m.h);
		}
#endif
	}

	chkerr();
}

}
