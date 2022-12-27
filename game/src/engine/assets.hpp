#pragma once

#include <string>
#include <map>

#include "gfx.hpp"
#include "../async.hpp"

namespace aoe {

class Engine;

class Assets final {
public:
	std::string path;
	std::map<io::DrsId, IdPoolRef> drs_ids;
	gfx::Tileset ts_ui;

	Assets(int id, Engine &e, const std::string &path);

	const gfx::ImageRef &at(io::DrsId) const;
private:
	void load_audio(Engine&, UI_TaskInfo&);
};

}
