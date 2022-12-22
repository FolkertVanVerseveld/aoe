#pragma once

#include <imgui.h>
#include "imgui_memory_editor.h"

#include "nominmax.hpp"

#include <tracy/Tracy.hpp>

namespace aoe {

class Debug {
	MemoryEditor mem_edit;
public:
	void show(bool &open);
};

}
