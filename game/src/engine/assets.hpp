#pragma once

#include <string>
#include <map>
#include <vector>

#include "gfx.hpp"
#include "../async.hpp"

namespace aoe {

class Engine;

class BackgroundColors final {
public:
	SDL_Color border[6];
};

class ImageSet final {
public:
	std::vector<IdPoolRef> imgs;

	friend bool operator<(const ImageSet &lhs, const ImageSet &rhs) {
		return lhs.imgs.front() < rhs.imgs.front();
	}
};

class Animation final {
public:
	std::unique_ptr<gfx::Image[]> images;
	unsigned image_count, all_count;
	bool dynamic;

	Animation() : images(), image_count(0), all_count(0), dynamic(false) {}

	void load(io::DRS&, const SDL_Palette*, io::DrsId);

	gfx::Image &subimage(unsigned index, unsigned player);
};

class Assets final {
	std::map<io::DrsId, ImageSet> drs_gifs;
public:
	std::string path;
	std::map<io::DrsId, IdPoolRef> drs_ids;
	std::map<io::DrsId, BackgroundColors> bkg_cols;
	gfx::Tileset ts_ui;
	Animation gif_cursors;

	Assets(int id, Engine &e, const std::string &path);

	const gfx::ImageRef &at(io::DrsId) const;
	const gfx::ImageRef &at(IdPoolRef) const;
	const ImageSet &anim_at(io::DrsId) const;
private:
	void load_gfx(Engine&, UI_TaskInfo&);
	void load_audio(Engine&, UI_TaskInfo&);
	void load_str(Engine&, UI_TaskInfo&);

	void add_gifs(gfx::ImagePacker &p, Animation&, io::DrsId);
};

}
