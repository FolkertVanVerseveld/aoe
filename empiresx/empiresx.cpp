/* Copyright 2016-2020 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Simple quick and dirty AOE demo using my own cmake-sdl2-template
 *
 * See INSTALL for instructions
 *
 * This is just a quick template to get started. Most code is quickly hacked together and could be made more consistent but I won't bother as long as it fucking works
 */

#include <cctype>
#include <cassert>
#include <cstdlib>
#include <cstddef>
#include <cstring>
#include <cmath>

#include <iostream>
#include <stdexcept>
#include <memory>
#include <vector>
#include <stack>
#include <map>
#include <algorithm>
#include <utility>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_syswm.h>

#include "os_macros.hpp"
#include "os.hpp"
#include "base/net.hpp"
#include "engine.hpp"
#include "font.hpp"
#include "menu.hpp"
#include "game.hpp"
#include "audio.hpp"
#include "string.hpp"

#if windows
#pragma comment(lib, "opengl32")
#endif

namespace genie {

const SDL_Rect menu_lobby_txt_cancel[screen_modes] = {
	{405 + (585 - 405) / 2, 440 + (470 - 440) / 2, 0, 0},
	{506 + (731 - 506) / 2, 550 + (587 - 550) / 2, 0, 0},
	{648 + (936 - 648) / 2, 704 + (752 - 704) / 2, 0, 0},
	{648 + (936 - 648) / 2, 704 + (752 - 704) / 2, 0, 0},
	{648 + (936 - 648) / 2, 704 + (752 - 704) / 2, 0, 0},
};

const SDL_Rect menu_lobby_border_cancel[screen_modes] = {
	{405, 440, 585 - 405, 470 - 440},
	{506, 550, 731 - 506, 587 - 550},
	{648, 704, 936 - 648, 752 - 704},
	{648, 704, 936 - 648, 752 - 704},
	{648, 704, 936 - 648, 752 - 704},
};

const SDL_Rect menu_lobby_lbl_chat[screen_modes] = {
	{16, 295, 0, 0},
	{18, 367, 0, 0},
	{22, 469, 0, 0},
	{22, 469, 0, 0},
	{22, 469, 0, 0},
};

const SDL_Rect menu_lobby_lbl_name[screen_modes] = {
	{32, 59, 0, 0},
	{38, 74, 0, 0},
	{46, 98, 0, 0},
	{46, 98, 0, 0},
	{46, 98, 0, 0},
};

const SDL_Rect menu_lobby_lbl_civ[screen_modes] = {
	{195, 59, 0, 0},
	{242, 74, 0, 0},
	{308, 98, 0, 0},
	{308, 98, 0, 0},
	{308, 98, 0, 0},
};

const SDL_Rect menu_lobby_lbl_player[screen_modes] = {
	{273, 59, 0, 0},
	{358, 74, 0, 0},
	{477, 98, 0, 0},
	{477, 98, 0, 0},
	{477, 98, 0, 0},
};

const SDL_Rect menu_lobby_border_chat[screen_modes] = {
	{10, 300, 410 - 10, 396 - 300},
	{12, 375, 512 - 12, 495 - 375},
	{16, 480, 656 - 16, 633 - 480},
	{16, 480, 656 - 16, 633 - 480},
	{16, 480, 656 - 16, 633 - 480},
};

const SDL_Rect menu_lobby_field_chat[screen_modes] = {
	{10, 402, 410 - 10, 425 - 402},
	{12, 502, 512 - 12, 525 - 502},
	{16, 643, 656 - 16, 666 - 643},
	{16, 643, 656 - 16, 666 - 643},
	{16, 643, 656 - 16, 666 - 643},
};

const SDL_Rect menu_lobby_border_start[screen_modes] = {
	{195, 440, 395 - 195, 470 - 440},
	{243, 550, 493 - 243, 587 - 550},
	{312, 704, 632 - 312, 752 - 704},
	{312, 704, 632 - 312, 752 - 704},
	{312, 704, 632 - 312, 752 - 704},
};

const SDL_Rect menu_lobby_txt_start[screen_modes] = {
	{195 + (395 - 195) / 2, 440 + (470 - 440) / 2},
	{243 + (493 - 243) / 2, 550 + (587 - 550) / 2},
	{312 + (632 - 312) / 2, 704 + (752 - 704) / 2},
	{312 + (632 - 312) / 2, 704 + (752 - 704) / 2},
	{312 + (632 - 312) / 2, 704 + (752 - 704) / 2},
};

/**
 * Minimal wrapper for a player that can chat in the lobby or in-game.
 */
class MenuPlayer final {
public:
	user_id id;
	std::string name;
	std::shared_ptr<Text> text; // couldn't use unique_ptr because copy ctor would be ill-formed, which we need for MenuLobbyState::dbuf

	// this ctor should only be used when looking up a menuplayer (e.g. std::set::find)
	MenuPlayer(user_id id) : id(id), name(), text() {}
	// this ctor should only be used for the network thread
	MenuPlayer(JoinUser &join) : id(join.id), name(join.nick()), text() {}

	MenuPlayer(JoinUser &join, SimpleRender &r, Font &f) : id(join.id), name(join.nick()), text(nullptr) {
		show(r, f);
	}

	void show(SimpleRender &r, Font &f) {
		if (text)
			return;

		text.reset(new Text(r, f, name, SDL_Color{0xff, 0xff, 0}));
	}

	/** Delete any graphical stuff associated with this player. */
	void hide() {
		text.reset();
	}

	friend bool operator<(const MenuPlayer &lhs, const MenuPlayer &rhs);
};

bool operator<(const MenuPlayer &lhs, const MenuPlayer &rhs) {
	return lhs.id < rhs.id;
}

class MenuLobbyState final {
public:
	std::deque<MenuLobbyText> chat;
	std::set<MenuPlayer> players;

	void dbuf(MenuLobbyState &s) {
		std::swap(chat, s.chat);

		// move all players
		while (!s.players.empty())
			players.emplace(std::move(s.players.extract(s.players.begin())).value());
	}

	/** Finalise double buffer swap. This must be called from the main thread. */
	void apply(std::deque<Text> &chat, SimpleRender &r, Font &f) {
		for (auto &x : this->chat)
			chat.emplace_front(r, eng->assets->fnt_default, x.text, x.col);

		this->chat.clear();
	}
};

/** High-level menu lobby state wrapper that only cares about user interface user representation. */
class UIPlayerState final : public MultiplayerCallback {
public:
	std::recursive_mutex mut;
	std::deque<Text> txtchat;
	MenuLobbyState state_now, state_next;
	bool host;
	int running;
	StartMatch settings;

	UIPlayerState(bool host) : mut(), txtchat(), state_now(), state_next(), host(host), running(-1), settings() {}

	/** Swap internal state to provide a `double buffering' like system */
	bool dbuf(SimpleRender &r) {
		std::lock_guard<std::recursive_mutex> lock(mut);
		state_now.dbuf(state_next);
		state_now.apply(txtchat, r, eng->assets->fnt_default);

		if (running == 0) {
			running = 1;
			return true;
		}

		return false;
	}

	void clearchat() {
		std::lock_guard<std::recursive_mutex> lock(mut);
		state_now.chat.clear();
		txtchat.clear();
	}

	/** Draw all chat items that fit within the specified bounds. */
	void paint(SimpleRender &r, int left, int start, int end) {
		unsigned i = 0;

		for (auto &x : txtchat) {
			int y = start - 20 * i++;
			if (y <= end)
				break;
			x.paint(r, left, y);
		}
	}

	void chat(const std::string &str, SDL_Color col) {
		std::lock_guard<std::recursive_mutex> lock(mut);
		state_next.chat.emplace_back(str, col);
	}

	void chat(const TextMsg &msg) {
		chat(msg.from, msg.str());
	}

	void chat(user_id from, const std::string &text) {
		std::lock_guard<std::recursive_mutex> lock(mut);

		// prepend name for message if it originated from a real user
		if (from || host) {
			auto search = state_now.players.find(from);
			assert(search != state_now.players.end());

			state_next.chat.emplace_back(search->name + ": " + text, SDL_Color{0xff, 0xff, 0});
		} else {
			chat(text, SDL_Color{0xff, 0, 0});
		}

		// FIXME should be thread-safe
		check_taunt(text);
	}

	void join(JoinUser &usr) {
		std::lock_guard<std::recursive_mutex> lock(mut);

		state_next.players.emplace(usr);
		chat(usr.nick() + " has joined", SDL_Color{0, 0xff, 0});
	}

	void leave(user_id id) {
		std::lock_guard<std::recursive_mutex> lock(mut);

		auto search = state_now.players.find(id);
		assert(search != state_now.players.end());
		chat(search->name + " has left", SDL_Color{0xff, 0, 0});

		state_now.players.erase(id);
	}

	void start(const StartMatch &settings) {
		std::lock_guard<std::recursive_mutex> lock(mut);

		assert(running == -1);
		running = 0;

		this->settings = settings;
	}
};

// FIXME dynamically determine which type of border (egyptian, greek, ...) needs to be used
res_id res_borders[screen_modes] = {
	50733,
	50737,
	50741,
	50741,
	50741,
};

const SDL_Rect menu_game_field_chat[screen_modes] = {
	{4, 354 - 30, 300, 23},
	{4, 474 - 30, 300, 23},
	{4, 642 - 30, 300, 23},
	{4, 642 - 30, 300, 23},
	{4, 642 - 30, 300, 23},
};

class MenuLobby;

/**
 * Direct representation wrapper for all visual elements in the world that are
 * visible within the visible screen area.
 */
class Viewport final {
public:
	game::Box2<float> bounds;

	std::vector<game::Particle*> particles;
	unsigned invalidate;

	static constexpr unsigned invalidate_static = 0x01;
	static constexpr unsigned invalidate_all = 0x01;

	float move_speed = 0.5f; // TODO playtest movement speed factor
	ConfigScreenMode mode;
	game::World &world;
	unsigned selected = 0;

	Viewport(game::World &world)
		: bounds(), particles(), invalidate(invalidate_all)
		, mode(eng->w->render().mode), world(world) {}

private:
	/** Ensure that the visual state is consistent with the associated world. */
	void update() {
		if (!invalidate)
			return;

		if (invalidate & invalidate_static) {
			particles.clear();
			world.query_static(particles, bounds);

			// maintain z-order by sorting all selected objects such that the upper units are drawn first
			std::sort(particles.begin(), particles.end(), [](game::Particle *lhs, game::Particle *rhs) {
				return lhs->scr.top + lhs->hotspot_y < rhs->scr.top + rhs->hotspot_y;
			});
		}

		invalidate = 0;
	}

public:
	/** Force refresh the visual state */
	void populate() {
		invalidate = invalidate_all;
		update();
	}

	void idle(game::World &world, int dx, int dy, unsigned ms) {
		auto &r = eng->w->render();
		ConfigScreenMode next_mode = r.mode;

		// we cannot check directly if the game mode has changed.
		// if it did, recompute the visual state.
		if (mode != next_mode) {
			invalidate = invalidate_all;
			mode = next_mode;
		}

		if (!dx && !dy) {
			update();
			return;
		}

		float angle, fdx, fdy, speed = move_speed * ms;

		angle = atan2f(static_cast<float>(dy), static_cast<float>(dx));
		fdx = cos(angle) * speed;
		fdy = sin(angle) * speed;

		bounds.left += fdx;
		bounds.top += fdy;
		auto &rel_bnds = r.dim.rel_bnds;
		bounds.w = static_cast<float>(rel_bnds.w);
		bounds.h = static_cast<float>(rel_bnds.h);

		// FIXME lock viewport bounds

		invalidate |= invalidate_static;
		update();
	}

	void reset() {
		bounds.left = bounds.top = 0;
	}

	void paint() {
		for (auto &x : particles)
			x->draw(static_cast<int>(-bounds.left), static_cast<int>(-bounds.top));
	}
};

class GameEvent {
public:
	GameEvent();
	virtual ~GameEvent() {}

	virtual void apply(game::World &world) = 0;
};

class GameEvents final {
	std::recursive_mutex mut;
	std::deque<std::unique_ptr<GameEvent>> events_now, events_next;
public:
	void dbuf() {
		std::lock_guard<std::recursive_mutex> lock(mut);
		events_now.swap(events_next);
	}

	void apply(game::World &world) {
		for (auto &e : events_now)
			e->apply(world);

		events_now.clear();
	}

	void idle(game::World &world) {
		dbuf();
		apply(world);
	}

	void push(GameEvent *e) {
		std::lock_guard<std::recursive_mutex> lock(mut);
		events_next.emplace_back(e);
	}
};

class MenuGame final : public Menu, public ui::InteractableCallback, ui::InputCallback, public game::Game {
	ImageCache img;
	bool host, started;

	ui::InputField *f_chat;

	static constexpr unsigned key_right = 0x01;
	static constexpr unsigned key_up    = 0x02;
	static constexpr unsigned key_left  = 0x04;
	static constexpr unsigned key_down  = 0x08;

	unsigned key_state;

	// lock to ensure access variables below are thread-safe
	std::recursive_mutex mut;
	UIPlayerState *playerstate;
	Viewport view;
	GameEvents events;
public:
	MenuGame(MenuLobby *lobby, SimpleRender &r, Multiplayer *mp, UIPlayerState *state, bool host, const StartMatch &settings)
		: Menu(MenuId::selectnav, r, eng->assets->fnt_title, "Game", SDL_Color{0xff, 0xff, 0xff}, true), Game(host ? game::GameMode::multiplayer_host : game::GameMode::multiplayer_client, lobby, mp, settings)
		, img(), host(host), started(false)
		, f_chat(nullptr), key_state(0)
		, mut()
		, playerstate(state) // copy state_now and state_next and txtchat from menulobby
		, view(world), events()
	{
		cache = &img;
		world.populate(settings.slave_count);
		view.populate();

		add_field(f_chat = new ui::InputField(0, *this, ui::InputType::text, "", r, eng->assets->fnt_default, SDL_Color{0xff, 0xff, 0xff}, menu_game_field_chat, r.mode, pal, bkg, true));
		if (!host)
			((MultiplayerClient*)mp)->set_gcb(this, (uint16_t)playerstate->state_now.players.size(), (uint16_t)lcg.next());
		else
			((MultiplayerHost*)mp)->set_gcb(this);

		jukebox.play(MusicId::game);
	}

private:
	/** Move visible area on screen at a consistent speed. */
	void update_viewport(unsigned ms) {
		int dx = 0, dy = 0;

		if (key_state & key_right)
			++dx;
		if (key_state & key_up)
			--dy;
		if (key_state & key_left)
			--dx;
		if (key_state & key_down)
			++dy;

		view.idle(this->world, dx, dy, ms);
	}
public:
	void idle(Uint32 ms) override {
		std::lock_guard<std::recursive_mutex> lock(mut);
		playerstate->dbuf(r);

		if (host && !started)
			started = ((MultiplayerHost*)mp)->try_start();

		if (started) {
			Game::step(ms);
			events.idle(world);
		}

		update_viewport(ms);
	}

	bool keydown(int ch) override {
		switch (ch) {
		case SDLK_RIGHT:
			key_state |= key_right;
			break;
		case SDLK_UP:
			key_state |= key_up;
			break;
		case SDLK_LEFT:
			key_state |= key_left;
			break;
		case SDLK_DOWN:
			key_state |= key_down;
			break;
		}

		if (!f_chat->focus() && ch == 't') {
			key_state = 0;
			f_chat->focus(true);
			return true;
		}

		return Menu::keydown(ch);
	}

	bool keyup(int ch) override {
		switch (ch) {
		case SDLK_RIGHT:
			key_state &= ~key_right;
			break;
		case SDLK_UP:
			key_state &= ~key_up;
			break;
		case SDLK_LEFT:
			key_state &= ~key_left;
			break;
		case SDLK_DOWN:
			key_state &= ~key_down;
			break;
		case SDLK_HOME:
			view.reset();
			break;
		case SDLK_ESCAPE:
			interacted(0);
			return true;
		}

		return Menu::keyup(ch);
	}

	void interacted(unsigned id) override {
		jukebox.sfx(SfxId::button4);
		// TODO support leaving the game, but not the lobby
		// this can get complicated because it will continue to receive
		// game events that are not handled because the user has resigned
		nav->quit(2);
	}

	bool input(unsigned id, ui::InputField &field) override {
		std::lock_guard<std::recursive_mutex> lock(mut);

		auto s = field.text();
		if (!s.empty()) {
			if (s == "/clear")
				playerstate->clearchat();
			else
				return mp->chat(s);
		}
		return true;
	}

private:
	void paint_tiles() {
		Animation &desert_tiles = img.get(15000);

		int left = static_cast<int>(-view.bounds.left), top = static_cast<int>(-view.bounds.top);

		for (unsigned ty = 0; ty < world.map.h; ++ty) {
			for (unsigned tx = 0; tx < world.map.w; ++tx) {
				int x, y;
				tile_to_scr(x, y, static_cast<int>(tx), static_cast<int>(ty));

				unsigned tile = world.map.tiles[ty * world.map.w + tx];

				desert_tiles.subimage(tile).draw(r, left + x, top + y);
			}
		}
	}

	void paint_hud_borders() {
		// figure out which border we need
		res_id res_border = res_borders[(unsigned)r.mode];
		Animation &anim_border = img.get(res_border);

		int left = 0;
		Image &img_top = anim_border.subimage(0), &img_bottom = anim_border.subimage(1);

		int top = r.dim.rel_bnds.h - img_bottom.texture.height;

		if (!mode_is_legacy(r.mode)) {
			left = (r.dim.rel_bnds.w - img_top.texture.width) / 2;

			int rep_left, rep_top, rep_right, rep_bottom;

			rep_left = left + img_bottom.texture.width;
			rep_top = top;
			rep_right = r.dim.rel_bnds.w;
			rep_bottom = r.dim.rel_bnds.h;

			int rep_w = 516 - 300;//735 - 517;
			int sx = 300 + (1024 - 953);

			// draw right side
			for (int x = rep_left; x < rep_right; x += rep_w) {
				img_top.draw(r, x, 0, rep_w, img_top.texture.height, sx, 0);
				img_bottom.draw(r, x, rep_top, rep_w, img_bottom.texture.height, sx, 0);
			}

			// draw left side (till 80)
			rep_right = left + 80 + (1024 - 953);

			for (int x = rep_right - rep_w; x >= -rep_w; x -= rep_w) {
				img_top.draw(r, x, 0, rep_w, img_top.texture.height, sx, 0);
				img_bottom.draw(r, x, rep_top, rep_w, img_bottom.texture.height, sx, 0);
			}
		}

		img_top.draw(r, left, 0);
		img_bottom.draw(r, left, top);

		playerstate->paint(r, 8, f_chat->bounds().y - 20, img_top.texture.height);
	}

public:
	void new_player(const CreatePlayer &create) override {
		std::lock_guard<std::recursive_mutex> lock(mut);
		players.emplace(create.id, create.str());
	}

	void assign_player(const AssignSlave &assign) override {
		std::lock_guard<std::recursive_mutex> lock(mut);

		assert(players.find(assign.to) != players.end());
		usertbl.emplace(assign.from, assign.to);
	}

	void paint() override {
		r.color({0, 0, 0, SDL_ALPHA_OPAQUE});
		r.clear();

		paint_tiles();
		view.paint();

		paint_hud_borders();
		paint_details(0);
	}
};

class MenuLobby final : public Menu, public ui::InteractableCallback, public ui::InputCallback {
	std::unique_ptr<Multiplayer> mp;
	bool host;
	int running;
	ui::Border *bkg_chat;
	ui::InputField *f_chat;
	ui::Label *lbl_name;

	// current state. these vars need not be thread-safe, since read and writes to these vars are only allowed from the main thread anyway
	UIPlayerState state;
	MenuGame *game;

	static constexpr unsigned lst_player_max = 8;
public:
	MenuLobby(SimpleRender &r, const std::string &name, uint32_t addr, uint16_t port, bool host = true)
		: Menu(MenuId::multiplayer, r, eng->assets->fnt_title, /*host ? "Multiplayer - Host" : "Multiplayer - Client"*/eng->assets->open_str(LangId::title_multiplayer), SDL_Color{ 0xff, 0xff, 0xff })
		, mp(), host(host), running(-1), bkg_chat(NULL), f_chat(NULL), lbl_name(nullptr), state(host)
		, game(nullptr)
	{
		Font &fnt = eng->assets->fnt_button;
		SDL_Color fg{bkg.text[0], bkg.text[1], bkg.text[2], 0xff}, bg{bkg.text[3], bkg.text[4], bkg.text[5], 0xff};
		ConfigScreenMode mode = eng->w->render().mode;

		ui_objs.emplace_back(new ui::Label(r, eng->assets->fnt_button, eng->assets->open_str(LangId::lobby_chat), menu_lobby_lbl_chat, eng->w->render().mode, pal, bkg, ui::HAlign::left, ui::VAlign::bottom, true, false));
		ui_objs.emplace_back(lbl_name = new ui::Label(r, eng->assets->fnt_button, eng->assets->open_str(LangId::lobby_name), menu_lobby_lbl_name, eng->w->render().mode, pal, bkg));
		ui_objs.emplace_back(new ui::Label(r, eng->assets->fnt_button, eng->assets->open_str(LangId::lobby_civ), menu_lobby_lbl_civ, eng->w->render().mode, pal, bkg));
		ui_objs.emplace_back(bkg_chat = new ui::Border(menu_lobby_border_chat, mode, pal, bkg, ui::BorderType::field, false));

		add_btn(new ui::Button(0, *this, r, fnt, eng->assets->open_str(LangId::btn_cancel), fg, bg, menu_lobby_txt_cancel, menu_lobby_border_cancel, pal, bkg, mode));
		if (host)
			add_btn(new ui::Button(1, *this, r, fnt, eng->assets->open_str(LangId::btn_lobby_start), fg, bg, menu_lobby_txt_start, menu_lobby_border_start, pal, bkg, mode));

		add_field(f_chat = new ui::InputField(0, *this, ui::InputType::text, "", r, eng->assets->fnt_default, SDL_Color{0xff, 0xff, 0}, menu_lobby_field_chat, mode, pal, bkg));
		resize(mode, mode);

		if (!host) {
			state.state_now.chat.emplace_back("Connecting to server...", SDL_Color{0xff, 0, 0});
		} else {
			JoinUser usr{0, name.c_str()};
			state.state_now.players.emplace(usr, r, eng->assets->fnt_default);
		}

		mp.reset(host ? (Multiplayer*)new MultiplayerHost(state, name, port) : (Multiplayer*)new MultiplayerClient(state, name, addr, port));
	}

	~MenuLobby() override {
		mp->dispose();
	}

	void idle(Uint32) override {
		if (state.dbuf(r))
			nav->go_to(game = new MenuGame(this, r, mp.get(), &state, host, state.settings));
	}

	bool keyup(int ch) override {
		switch (ch) {
		case SDLK_ESCAPE:
			interacted(0);
			return true;
		}

		return Menu::keyup(ch);
	}

	void interacted(unsigned id) override;

	bool input(unsigned id, ui::InputField &f) override {
		switch (id) {
		case 0:
			{
				std::lock_guard<std::recursive_mutex> lock(state.mut);
				auto s = f.text();
				if (!s.empty()) {
					if (s == "/clear")
						state.clearchat();
					else
						return mp->chat(s);
				}
			}
			break;
		}
		return true;
	}

	void paint() override {
		Menu::paint();

		unsigned i = 0;
		SDL_Rect bnds(bkg_chat->bounds());

		state.paint(r, bnds.x + 8, bnds.y + bnds.h - 18, bnds.y + 4);

		// no need to lock state_now, since it will never be written to during paint()
		// always draw ourself first
		if (state.state_now.players.empty())
			return;

		user_id self = host ? 0 : mp->self;

		auto search = state.state_now.players.find(self);
		if (search == state.state_now.players.end()) {
			// player not added yet, skip frame
			return;
		}

		bnds = lbl_name->bounds();

		// 640: 33, 90
		// 800: 40, 114

		const_cast<MenuPlayer&>(*search).show(r, eng->assets->fnt_default);
		search->text->paint(r, bnds.x + 3, bnds.y + 30);

		i = 1;

		for (auto &p : state.state_now.players) {
			if (p.id == self)
				continue;

			if (i >= lst_player_max) {
				const_cast<MenuPlayer&>(p).hide();
				continue;
			}

			const_cast<MenuPlayer&>(p).show(r, eng->assets->fnt_default);
			p.text->paint(r, bnds.x + 3, bnds.y + 30 + i++ * 20);
		}
	}

	void stop_game() {
		std::lock_guard<std::recursive_mutex> lock(state.mut);
		game = nullptr;
	}
};

void MenuLobby::interacted(unsigned id) {
	jukebox.sfx(SfxId::button4);

	switch (id) {
	case 0:
		nav->quit(1);
		break;
	case 1:
		if (host)
			((MultiplayerHost*)mp.get())->prepare_match();
		break;
	}
}

void menu_lobby_stop_game(MenuLobby *lobby) {
	lobby->stop_game();
}

const SDL_Rect menu_multi_lbl_name[screen_modes] = {
	{26, 78, 0, 0},
	{31, 96, 0, 0},
	{38, 123, 0, 0},
	{38, 123, 0, 0},
	{38, 123, 0, 0},
};

const SDL_Rect menu_multi_field_name[screen_modes] = {
	{26, 78 + 20, 200 - 26, 20},
	{31, 96 + 30, 250 - 31, 30},
	{38, 123 + 40, 320 - 38, 40},
	{38, 123 + 40, 320 - 38, 40},
	{38, 123 + 40, 320 - 38, 40},
};

const SDL_Rect menu_multi_lbl_port[screen_modes] = {
	{480, 78, 0, 0},
	{600, 96, 0, 0},
	{768, 123, 0, 0},
	{768, 123, 0, 0},
	{768, 123, 0, 0},
};

const SDL_Rect menu_multi_field_port[screen_modes] = {
	{480, 78 + 20, 580 - 480, 20},
	{600, 96 + 30, 725 - 600, 30},
	{768, 123 + 40, 928 - 768, 40},
	{768, 123 + 40, 928 - 768, 40},
	{768, 123 + 40, 928 - 768, 40},
};

const SDL_Rect menu_multi_lbl_ip[screen_modes] = {
	{288, 78, 0, 0},
	{360, 96, 0, 0},
	{460, 123, 0, 0},
	{460, 123, 0, 0},
	{460, 123, 0, 0},
};

const SDL_Rect menu_multi_field_ip[screen_modes] = {
	{288, 78 + 20, 448 - 288, 20},
	{360, 96 + 30, 560 - 360, 30},
	{460, 123 + 40, 716 - 460, 40},
	{460, 123 + 40, 716 - 460, 40},
	{460, 123 + 40, 716 - 460, 40},
};

const SDL_Rect menu_multi_btn_txt_host[screen_modes] = {
	{220 + (420 - 220) / 2, 440 + (470 - 440) / 2, 0, 0},
	{275 + (525 - 275) / 2, 550 + (587 - 550) / 2, 0, 0},
	{352 + (672 - 352) / 2, 704 + (752 - 704) / 2, 0, 0},
	{352 + (672 - 352) / 2, 704 + (752 - 704) / 2, 0, 0},
	{352 + (672 - 352) / 2, 704 + (752 - 704) / 2, 0, 0},
};

const SDL_Rect menu_multi_btn_border_host[screen_modes] = {
	{220, 440, 420 - 220, 470 - 440},
	{275, 550, 525 - 275, 587 - 550},
	{352, 704, 672 - 352, 752 - 704},
	{352, 704, 672 - 352, 752 - 704},
	{352, 704, 672 - 352, 752 - 704},
};

const SDL_Rect menu_multi_btn_txt_join[screen_modes] = {
	{10 + (210 - 10) / 2, 440 + (470 - 440) / 2, 0, 0},
	{12 + (262 - 12) / 2, 550 + (587 - 550) / 2, 0, 0},
	{16 + (336 - 16) / 2, 704 + (752 - 704) / 2, 0, 0},
	{16 + (336 - 16) / 2, 704 + (752 - 704) / 2, 0, 0},
	{16 + (336 - 16) / 2, 704 + (752 - 704) / 2, 0, 0},
};

const SDL_Rect menu_multi_btn_border_join[screen_modes] = {
	{10, 440, 210 - 10, 470 - 440},
	{12, 550, 262 - 12, 587 - 550},
	{16, 704, 336 - 16, 752 - 704},
	{16, 704, 336 - 16, 752 - 704},
	{16, 704, 336 - 16, 752 - 704},
};

const SDL_Rect menu_multi_btn_txt_cancel[screen_modes] = {
	{529, 456, 0, 0},
	{658, 569, 0, 0},
	{846, 729, 0, 0},
	{846, 729, 0, 0},
	{846, 729, 0, 0},
};

const SDL_Rect menu_multi_btn_border_cancel[screen_modes] = {
	{430, 440, 630 - 430, 470 - 440},
	{537, 550, 787 - 537, 587 - 550},
	{688, 704, 1008 - 688, 752 - 704},
	{688, 704, 1008 - 688, 752 - 704},
	{688, 704, 1008 - 688, 752 - 704},
};

class MenuMultiplayer final : public Menu, public ui::InteractableCallback, public ui::InputCallback {
	uint16_t port;
	std::string name;
	uint32_t ip;
	ui::InputField *f_name, *f_port, *f_ip;
public:
	MenuMultiplayer(SimpleRender &r)
		// we skip the connection type menu 9611, because we don't support serial connection or microsoft game zone anyway
		: Menu(MenuId::multiplayer, r, eng->assets->fnt_title, eng->assets->open_str(LangId::title_multiplayer_servers), SDL_Color{ 0xff, 0xff, 0xff })
		, port(25659), name(), ip(0), f_name(nullptr), f_port(nullptr), f_ip(nullptr)
	{
		Font &fnt = eng->assets->fnt_button;
		SDL_Color fg{bkg.text[0], bkg.text[1], bkg.text[2], 0xff}, bg{bkg.text[3], bkg.text[4], bkg.text[5], 0xff};
		ConfigScreenMode mode = eng->w->render().mode;

		ui_objs.emplace_back(new ui::Label(r, eng->assets->fnt_button, "Name", menu_multi_lbl_name, eng->w->render().mode, pal, bkg));
		ui_objs.emplace_back(new ui::Label(r, eng->assets->fnt_button, "Port", menu_multi_lbl_port, eng->w->render().mode, pal, bkg));
		ui_objs.emplace_back(new ui::Label(r, eng->assets->fnt_button, "Address", menu_multi_lbl_ip, eng->w->render().mode, pal, bkg));

		add_btn(new ui::Button(1, *this, r, fnt, "(C) " + eng->assets->open_str(LangId::multiplayer_host), fg, bg, menu_multi_btn_txt_host, menu_multi_btn_border_host, pal, bkg, mode));
		add_btn(new ui::Button(2, *this, r, fnt, "(J) " + eng->assets->open_str(LangId::multiplayer_join), fg, bg, menu_multi_btn_txt_join, menu_multi_btn_border_join, pal, bkg, mode));
		add_btn(new ui::Button(0, *this, r, fnt, "(Q) " + eng->assets->open_str(LangId::btn_cancel), fg, bg, menu_multi_btn_txt_cancel, menu_multi_btn_border_cancel, pal, bkg, mode));

		add_field(f_port = new ui::InputField(0, *this, ui::InputType::port, std::to_string(port), r, eng->assets->fnt_default, SDL_Color{0xff, 0xff, 0xff}, menu_multi_field_port, mode, pal, bkg));
		add_field(f_name = new ui::InputField(1, *this, ui::InputType::text, "you", r, eng->assets->fnt_default, SDL_Color{0xff, 0xff, 0xff}, menu_multi_field_name, mode, pal, bkg));
		add_field(f_ip = new ui::InputField(2, *this, ui::InputType::ip, "127.0.0.1", r, eng->assets->fnt_default, SDL_Color{0xff, 0xff, 0xff}, menu_multi_field_ip, mode, pal, bkg));
	}

	bool keyup(int ch) override {
		if (Menu::keyup(ch))
			return true;

		switch (ch) {
		case 'c':
		case 'C':
			interacted(1);
			break;
		case 'j':
		case 'J':
			interacted(2);
			break;
		case 'q':
		case 'Q':
		case SDLK_ESCAPE:
			interacted(0);
			break;
		}
		return true;
	}

private:
	bool valid() {
		name = f_name->text();
		port = f_port->port();

		bool good = true;
		f_name->error = f_ip->error = f_port->error = false;

		if (!f_ip->ip(ip)) {
			good = false;
			f_ip->error = true;
		}

		if (port < 1 || port > 65535) {
			good = false;
			f_port->error = true;
		}

		if (name.empty()) {
			good = false;
			f_name->error = true;
		}

		if (!good) {
			jukebox.sfx(SfxId::error);
			return false;
		}

		return true;
	}
public:
	void interacted(unsigned id) override {
		switch (id) {
		case 0:
			jukebox.sfx(SfxId::button4);
			nav->quit(1);
			break;
		case 1:
		case 2:
			if (valid()) {
				jukebox.sfx(SfxId::button4);
				go_to(new MenuLobby(r, name, ip, port, id == 1));
			}
			break;
		}
	}

	bool input(unsigned, ui::InputField&) {
		return false;
	}
};

const SDL_Rect menu_ext_settings_lbl_mode[screen_modes] = {
	{320, 198 - 30, 133, 13},
	{400, 248 - 40, 133, 13},
	{512, 316 - 50, 133, 13},
	{512, 316 - 50, 133, 13},
	{512, 316 - 50, 133, 13},
};

const SDL_Rect menu_start_btn_txt_start[screen_modes] = {
	{320, 198, 133, 13},
	{400, 248, 133, 13},
	{512, 316, 133, 13},
	{512, 316, 133, 13},
	{512, 316, 133, 13},
};

const SDL_Rect menu_start_btn_border_start[screen_modes] = {
	{170, 178, 470 - 170, 218 - 178},
	{212, 222, 587 - 212, 272 - 222},
	{272, 284, 752 - 272, 348 - 284},
	{272, 284, 752 - 272, 348 - 284},
	{272, 284, 752 - 272, 348 - 284},
};

const SDL_Rect menu_start_btn_txt_multi[screen_modes] = {
	{320, 249, 114, 13},
	{400, 311, 114, 13},
	{512, 396, 114, 13},
	{512, 396, 114, 13},
	{512, 396, 114, 13},
};

const SDL_Rect menu_start_btn_border_multi[screen_modes] = {
	{170, 228, 470 - 170, 268 - 228},
	{212, 285, 587 - 212, 335 - 285},
	{272, 364, 752 - 272, 428 - 364},
	{272, 364, 752 - 272, 428 - 364},
	{272, 364, 752 - 272, 428 - 364},
};

const SDL_Rect menu_start_btn_txt_help[screen_modes] = {
	{320, 299, 44, 13},
	{400, 374, 44, 13},
	{512, 478, 44, 13},
	{512, 478, 44, 13},
	{512, 478, 44, 13},
};

const SDL_Rect menu_start_btn_border_help[screen_modes] = {
	{170, 278, 470 - 170, 318 - 278},
	{212, 347, 587 - 212, 397 - 347},
	{272, 444, 752 - 272, 508 - 444},
	{272, 444, 752 - 272, 508 - 444},
	{272, 444, 752 - 272, 508 - 444},
};

const SDL_Rect menu_start_btn_txt_editor[screen_modes] = {
	{320, 350, 163, 13},
	{400, 437, 163, 13},
	{512, 558, 163, 13},
	{512, 558, 163, 13},
	{512, 558, 163, 13},
};

const SDL_Rect menu_start_btn_border_editor[screen_modes] = {
	{170, 328, 470 - 170, 368 - 328},
	{212, 410, 587 - 212, 460 - 410},
	{272, 524, 752 - 272, 588 - 524},
	{272, 524, 752 - 272, 588 - 524},
	{272, 524, 752 - 272, 588 - 524},
};

const SDL_Rect menu_start_btn_txt_quit[screen_modes] = {
	{320, 399, 37, 13},
	{400, 498, 37, 13},
	{512, 637, 37, 13},
	{512, 637, 37, 13},
	{512, 637, 37, 13},
};

const SDL_Rect menu_start_btn_border_quit[screen_modes] = {
	{170, 378, 470 - 170, 418 - 378},
	{212, 472, 587 - 212, 522 - 472},
	{272, 604, 752 - 272, 668 - 604},
	{272, 604, 752 - 272, 668 - 604},
	{272, 604, 752 - 272, 668 - 604},
};

/** Custom help and game settings menu. */
class MenuExtSettings final : public Menu, public ui::InteractableCallback {
public:
	MenuExtSettings(SimpleRender &r)
		: Menu(MenuId::selectnav, r, eng->assets->fnt_title, "Help and Global game settings", SDL_Color{0xff, 0xff,0xff}, true)
	{
		Font &fnt = eng->assets->fnt_button;
		SDL_Color fg{bkg.text[0], bkg.text[1], bkg.text[2], 0xff}, bg{bkg.text[3], bkg.text[4], bkg.text[5], 0xff};
		ConfigScreenMode mode = eng->w->render().mode;

		ui_objs.emplace_back(new ui::Label(r, eng->assets->fnt_button, "Video resolution", menu_ext_settings_lbl_mode, eng->w->render().mode, pal, bkg, ui::HAlign::center, ui::VAlign::bottom, true, true));

		add_btn(new ui::Button(1, *this, r, fnt, "(1) " + eng->assets->open_str(LangId::mode_640_480), fg, bg, menu_start_btn_txt_start, menu_start_btn_border_start, pal, bkg, mode, ui::HAlign::center, ui::VAlign::middle, true, true));
		add_btn(new ui::Button(2, *this, r, fnt, "(2) " + eng->assets->open_str(LangId::mode_800_600), fg, bg, menu_start_btn_txt_multi, menu_start_btn_border_multi, pal, bkg, mode, ui::HAlign::center, ui::VAlign::middle, true, true));
		add_btn(new ui::Button(3, *this, r, fnt, "(3) " + eng->assets->open_str(LangId::mode_1024_768), fg, bg, menu_start_btn_txt_help, menu_start_btn_border_help, pal, bkg, mode, ui::HAlign::center, ui::VAlign::middle, true, true));
		add_btn(new ui::Button(4, *this, r, fnt, "(4/F) Fullscreen", fg, bg, menu_start_btn_txt_editor, menu_start_btn_border_editor, pal, bkg, mode, ui::HAlign::center, ui::VAlign::middle, true, true));
		add_btn(new ui::Button(0, *this, r, fnt, "(Q) " + eng->assets->open_str(LangId::btn_back), fg, bg, menu_start_btn_txt_quit, menu_start_btn_border_quit, pal, bkg, mode, ui::HAlign::center, ui::VAlign::middle, true, true));

		resize(mode, mode);
	}

	bool keyup(int ch) override {
		switch (ch) {
		case '1':
			interacted(1);
			break;
		case '2':
			interacted(2);
			break;
		case '3':
			interacted(3);
			break;
		case '4':
		case 'f':
		case 'F':
			interacted(4);
			break;
		case 'q':
		case 'Q':
		case SDLK_ESCAPE:
			interacted(0);
			break;
		}
		return true;
	}

	void interacted(unsigned id) override {
		jukebox.sfx(SfxId::button4);

		switch (id) {
		case 0:
			nav->quit(1);
			break;
		case 1:
			eng->w->chmode(ConfigScreenMode::MODE_640_480);
			break;
		case 2:
			eng->w->chmode(ConfigScreenMode::MODE_800_600);
			break;
		case 3:
			eng->w->chmode(ConfigScreenMode::MODE_1024_768);
			break;
		case 4:
			eng->w->chmode(ConfigScreenMode::MODE_FULLSCREEN);
			break;
		}
	}
};

const SDL_Rect menu_start_lbl_copy3[screen_modes] = {
	{320, 480 - 10, 133, 13},
	{400, 600 - 10, 133, 13},
	{512, 800 - 10, 133, 13},
	{512, 800 - 10, 133, 13},
	{512, 800 - 10, 133, 13},
};

class MenuStart final : public Menu, public ui::InteractableCallback {
public:
	MenuStart(SimpleRender &r)
		: Menu(MenuId::start, r, eng->assets->fnt_title, eng->assets->open_str(LangId::title_main), SDL_Color{ 0xff, 0xff, 0xff })
	{
		Font &fnt = eng->assets->fnt_button;
		SDL_Color fg{bkg.text[0], bkg.text[1], bkg.text[2], 0xff}, bg{bkg.text[3], bkg.text[4], bkg.text[5], 0xff};
		ConfigScreenMode mode = eng->w->render().mode;

		// TODO add single player and scenario editor menus
		//add_btn(new ui::Button(0, *this, r, fnt, "(S) " + eng->assets->open_str(LangId::btn_singleplayer), fg, bg, menu_start_btn_txt_start, menu_start_btn_border_start, pal, bkg, mode));
		add_btn(new ui::Button(1, *this, r, fnt, "(M) " + eng->assets->open_str(LangId::btn_multiplayer), fg, bg, menu_start_btn_txt_multi, menu_start_btn_border_multi, pal, bkg, mode));
		add_btn(new ui::Button(2, *this, r, fnt, "(H) Help and settings", fg, bg, menu_start_btn_txt_help, menu_start_btn_border_help, pal, bkg, mode));
		//add_btn(new ui::Button(3, *this, r, fnt, "(E) " + eng->assets->open_str(LangId::btn_edit), fg, bg, menu_start_btn_txt_editor, menu_start_btn_border_editor, pal, bkg, mode));
		add_btn(new ui::Button(4, *this, r, fnt, "(Q) " + eng->assets->open_str(LangId::btn_exit), fg, bg, menu_start_btn_txt_quit, menu_start_btn_border_quit, pal, bkg, mode));

		ui_objs.emplace_back(new ui::Label(r, eng->assets->fnt_default, "© 1997 Microsoft & © 2016-2020 Folkert van Verseveld. Some rights reserved", menu_start_lbl_copy3, eng->w->render().mode, pal, bkg, ui::HAlign::center, ui::VAlign::bottom, true, true));

		resize(mode, mode);
		jukebox.play(MusicId::start);
	}

	bool keyup(int ch) override {
		switch (ch) {
		case 's':
		case 'S':
			//interacted(0);
			break;
		case 'm':
		case 'M':
			interacted(1);
			break;
		case 'h':
		case 'H':
			interacted(2);
			break;
		case 'e':
		case 'E':
			//interacted(3);
			break;
		case 'q':
		case 'Q':
		case SDLK_ESCAPE:
			interacted(4);
			break;
		}
		return true;
	}

	void interacted(unsigned id) override {
		jukebox.sfx(SfxId::button4);

		switch (id) {
		case 0:
			break;
		case 1:
			go_to(new MenuMultiplayer(r));
			break;
		case 2:
			go_to(new MenuExtSettings(r));
			break;
		case 3:
			break;
		case 4:
			nav->quit();
			break;
		}
	}

	void paint() override {
		paint_details(Menu::show_background | Menu::show_border);
	}
};

Navigator::Navigator(SimpleRender &r) : r(r), trace(), top() {
	trace.emplace_back(top = new MenuStart(r));
}

}

int main(int argc, char **argv)
{
	try {
		genie::Config cfg(argc, argv);

		std::cout << "hello " << genie::os.username << " on " << genie::os.compname << '!' << std::endl;

		genie::Engine eng(cfg);

		genie::SimpleRender &r = (genie::SimpleRender&)eng.w->render();
		genie::nav.reset(new genie::Navigator(r));

		genie::nav->mainloop();
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		genie::show_error(e.what());
		return 1;
	}

	return 0;
}

