#pragma once

#include <idpool.hpp>

namespace aoe {

enum class TaskFlags {
	// User may abort task.
	cancellable = 1 << 0,
	// UI blocks until task has been completed.
	blocking = 1 << 1,
	all = (1 << 2) - 1,
};

// XXX make int, uiint UI_TaskRef?

class UI_TaskError : public std::runtime_error {
public:
	explicit UI_TaskError(const std::string &msg) : std::runtime_error(msg) {}
	explicit UI_TaskError(const char *msg) : std::runtime_error(msg) {}
};

/** Wrapper for tracking tasks in Engine. */
struct UI_Task final {
	IdPoolRef id;
	TaskFlags flags;
	std::string title, desc;
	unsigned steps, total;

	UI_Task(IdPoolRef id, TaskFlags flags, const std::string &title, const std::string &desc, unsigned steps, unsigned total)
		: id(id), flags(flags), title(title), desc(desc), steps(steps), total(total) {}
};

/** Wrapper to manage ui task info. */
class UI_TaskInfo final {
	IdPoolRef ref;
	TaskFlags flags;
public:
	UI_TaskInfo(IdPoolRef ref, TaskFlags flags) : ref(ref), flags(flags) {}
	~UI_TaskInfo();

	IdPoolRef get_ref() { return ref; }
	constexpr TaskFlags get_flags() const noexcept { return flags; }

	void set_total(unsigned total);
	void set_desc(const std::string &s);
	void next();
	void next(const std::string &s);
};

}
