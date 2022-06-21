#pragma once

#include <mutex>
#include <memory>

namespace aoe {

class Engine final {
public:
	Engine();
	~Engine();

	int mainloop();
};

}
