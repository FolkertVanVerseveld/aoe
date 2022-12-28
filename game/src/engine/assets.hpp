#pragma once

#include <string>
#include <map>

#include "gfx.hpp"
#include "../async.hpp"

namespace aoe {

class Engine;

class BackgroundColors final {
public:
	SDL_Color border[6];
};

class Assets final {
public:
	std::string path;
	std::map<io::DrsId, IdPoolRef> drs_ids;
	std::map<io::DrsId, BackgroundColors> bkg_cols;
	gfx::Tileset ts_ui;

	Assets(int id, Engine &e, const std::string &path);

	const gfx::ImageRef &at(io::DrsId) const;
private:
	void load_audio(Engine&, UI_TaskInfo&);
};

}
