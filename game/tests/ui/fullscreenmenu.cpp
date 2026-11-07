#include "../../src/ui/fullscreenmenu.hpp"

#include <gtest/gtest.h>

namespace aoe {

extern ui::OrthogonalGroup opSinglePlayerHost[];

namespace ui {

TEST(UI, og_init) {
	OrthogonalGroup *og = mainMenu.orthogonal;
	ASSERT_EQ(og->fnActivate, StartMenuButtonActivate);
	ASSERT_EQ(og->activated, false);
}

TEST(UI, fsm_single_group_selected) {
	OrthogonalGroup *og = singleplayerHostMenu.orthogonal;
	unsigned ogCount = singleplayerHostMenu.ogCount;

	unsigned selected = 0;

	for (unsigned i = 0; i < ogCount; ++i) {
		OrthogonalGroup &g = og[i];

		for (unsigned j = 0; j < g.buttonCount; ++j) {
			MenuButton &btn = g.buttons[j];
			if (btn.state & (unsigned)MenuButtonState::selected)
				++selected;
		}
	}

	ASSERT_EQ(selected, 1);
}

TEST(UI, fsm_mouse_down) {
	// TODO stub audio
	ASSERT_EQ(singleplayerHostMenu.orthogonal, opSinglePlayerHost);

	GTEST_SKIP() << "stub";
}

}
}