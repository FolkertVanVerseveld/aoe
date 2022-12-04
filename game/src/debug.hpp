#pragma once

#include <imgui.h>
#include "imgui_memory_editor.h"

namespace aoe {

class Debug {
	MemoryEditor mem_edit;
public:
	void show(bool &open);
};

}
