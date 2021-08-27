#pragma once

#include "../imgui/imgui.h"
#include "../graphics.hpp"
#include "hud.hpp"

namespace genie {
namespace ui {

class TexturesWidget final {
public:
	bool visible;

	void display();
};

class DebugUI final {
	TexturesWidget tex;
public:
	DebugUI();

	void display();
};

class UI final {
	bool m_show_debug;
	DebugUI dbg;
public:
	UI();

	constexpr bool show_debug() const noexcept { return m_show_debug; }
	void show_debug(bool enabled);

	void display();
};

}
}
