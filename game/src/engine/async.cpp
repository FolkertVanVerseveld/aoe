#include "../engine.hpp"

#include <thread>

namespace aoe {

UI_TaskInfo::~UI_TaskInfo() {
	ZoneScoped;
	// just tell engine task has completed, we don't care if it succeeds
	std::lock_guard<std::mutex> lock(m_eng);
	if (eng)
		(void)eng->ui_async_stop(*this);
}

void UI_TaskInfo::set_total(unsigned total) {
	ZoneScoped;
	std::lock_guard<std::mutex> lock(m_eng);
	if (eng)
		eng->ui_async_set_total(*this, total);
}

void UI_TaskInfo::set_desc(const std::string &s) {
	ZoneScoped;
	std::lock_guard<std::mutex> lock(m_eng);
	if (eng)
		eng->ui_async_set_desc(*this, s);
}

void UI_TaskInfo::next() {
	ZoneScoped;
	std::lock_guard<std::mutex> lock(m_eng);
	if (eng)
		eng->ui_async_next(*this);
	std::this_thread::yield();
}

void UI_TaskInfo::next(const std::string &s) {
	ZoneScoped;
	std::lock_guard<std::mutex> lock(m_eng);
	if (eng)
		eng->ui_async_next(*this, s);
	std::this_thread::yield();
}

}
