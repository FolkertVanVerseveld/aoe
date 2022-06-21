#pragma once

#include <mutex>

namespace aoe {

class Engine final {
	bool show_demo;
public:
	Engine();
	~Engine();

	int mainloop();
private:
	void display();
	void show_menubar();
};

extern Engine *eng;
extern std::mutex m_eng;

}
