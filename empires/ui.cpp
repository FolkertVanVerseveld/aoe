/* Copyright 2016-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "ui.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#ifdef _WIN32
	#warning gfx primitives not supported yet
#else
	#include <SDL2/SDL2_gfxPrimitives.h>
#endif

#include <cassert>
#include <cstdio>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <strings.h>

#include <memory>
#include <string>
#include <stack>
#include <vector>

#include <genie/dbg.h>
#include <genie/def.h>
#include <genie/res.h>

#include "errno.h"
#include "cfg.h"
#include "drs.h"
#include "gfx.h"
#include "lang.h"
#include "math.h"
#include "sfx.h"
#include "cpn.h"
#include "genie.h"

#include "fs.hpp"
#include "image.hpp"
#include "game.hpp"
#include "editor.hpp"
#include "scenario.hpp"

extern struct pe_lib lib_lang;

const unsigned menu_bar_tbl[MAX_CIVILIZATION_COUNT] = {
	0, // egyptian
	1, // greek
	2, // babylonian
	0, // assyrian
	1, // minoan
	2, // hittite
	1, // phoenician
	0, // sumerian
	2, // persian
	3, // shang
	3, // yamato
	3, // choson
};

/** load c-string from language dll and wrap into c++ string */
std::string load_string(unsigned id)
{
	char buf[4096];
	load_string(&lib_lang, id, buf, sizeof buf);
	return std::string(buf);
}

class Menu;

/**
 * Global User Interface state handler.
 * This `state machine' determines when the screen has to be redisplayed
 * and does some navigation logic as well.
 */
class UI_State {
	unsigned state;

	static unsigned const DIRTY = 1;
	static unsigned const BUTTON_DOWN = 2;

	std::stack<std::shared_ptr<Menu>> navigation;
public:
	std::string file_selected;

	UI_State() : state(DIRTY), navigation(), file_selected() {}

	void mousedown(SDL_MouseButtonEvent *event);
	void mouseup(SDL_MouseButtonEvent *event);
	virtual void keydown(SDL_KeyboardEvent *event);
	virtual void keyup(SDL_KeyboardEvent *event);

	/* Push new menu onto navigation stack */
	void go_to(Menu *menu);

	void dirty() {
		state |= DIRTY;
	}

	void idle();
	void display();
	void dispose();
private:
	void update();
} ui_state;

void canvas_dirty() {
	ui_state.dirty();
}

/* Custom renderer */
Renderer::Renderer()
	: capture(NULL), tex(NULL)
	, state(), renderer(NULL), shade(100)
{
	if (!(pixels = malloc(pixels_count = MAX_BKG_WIDTH * MAX_BKG_HEIGHT * 3)))
		panic("Out of memory");

	state.emplace();
}

Renderer::~Renderer() {
	if (pixels)
		free(pixels);
	if (capture)
		SDL_FreeSurface(capture);
}

void Renderer::draw(SDL_Texture *tex, SDL_Surface *surf, int x, int y, unsigned w, unsigned h)
{
	if (!tex)
		return;

	RendererState &s = get_state();

	if (!w) w = surf->w;
	if (!h) h = surf->h;

	// repeat if w > surf->w or h > surf->h
	for (int top = y, bottom = y + h; top < bottom; top += surf->h) {
		for (int left = x, right = x + w; left < right; left += surf->w) {
		#define min(a, b) ((a) < (b) ? (a) : (b))
			int sw = min(surf->w, right - left), sh = min(surf->h, bottom - top);
			SDL_Rect src = {0, 0, sw, sh};
			SDL_Rect dst = {left - s.view_x, top - s.view_y, sw, sh};
		#undef min
			SDL_RenderCopy(renderer, tex, &src, &dst);
		}
	}
}

void Renderer::draw_selection(int x, int y, unsigned size)
{
	RendererState &s = get_state();
	col(255);

	int w = size, h = size * TILE_HEIGHT / TILE_WIDTH;

	SDL_Point points[5];
	points[0].x = x; points[0].y = y - h;
	points[1].x = x + w; points[1].y = y;
	points[2].x = x; points[2].y = y + h;
	points[3].x = x - w; points[3].y = y;
	points[4].x = x; points[4].y = y - h;

	// apply viewport
	points[0].x -= s.view_x; points[0].y -= s.view_y;
	points[1].x -= s.view_x; points[1].y -= s.view_y;
	points[2].x -= s.view_x; points[2].y -= s.view_y;
	points[3].x -= s.view_x; points[3].y -= s.view_y;
	points[4].x -= s.view_x; points[4].y -= s.view_y;

	SDL_RenderDrawLines(renderer, points, 5);
}

void Renderer::draw_rect(int x0, int y0, int x1, int y1)
{
	RendererState &s = get_state();
	SDL_Rect bounds = {x0 - s.view_x, y0 - s.view_y, x1 - x0, y1 - y0};
	SDL_RenderDrawRect(renderer, &bounds);
}

void Renderer::fill_rect(int x0, int y0, int x1, int y1)
{
	RendererState &s = get_state();
	SDL_Rect bounds = {x0 - s.view_x, y0 - s.view_y, x1 - x0, y1 - y0};
	SDL_RenderFillRect(renderer, &bounds);
}

void Renderer::save_screen() {
	SDL_Surface *screen = NULL;
	int err = 1;
	size_t needed = gfx_cfg.width * gfx_cfg.height * 3;
	SDL_Rect bnds = {0, 0, gfx_cfg.width, gfx_cfg.height};

	/*
	 * We could just panic if we can't allocate more pixels,
	 * but that's stupid because the game should not crash in a match
	 * if you just want to take a screenshot.
	 */
	if (needed > pixels_count) {
		void *new_pixels;

		if (!(new_pixels = realloc(pixels, needed)))
			goto fail;

		pixels = new_pixels;
		pixels_count = needed;
	}

	// FIXME support big endian byte order
	if (!(screen = SDL_CreateRGBSurface(0, gfx_cfg.width, gfx_cfg.height, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000)))
		goto fail;

	if (SDL_RenderReadPixels(renderer, &bnds, SDL_PIXELFORMAT_ARGB8888, screen->pixels, screen->pitch))
		goto fail;
	if (SDL_SaveBMP(screen, "empires.bmp"))
		goto fail;

	err = 0;
fail:
	if (err)
		show_error("Cannot take screen shot");
	if (screen)
		SDL_FreeSurface(screen);
}

void Renderer::read_screen() {
	if (capture)
		SDL_FreeSurface(capture);
	// FIXME support big endian byte order
	if (!(capture = SDL_CreateRGBSurface(0, gfx_cfg.width, gfx_cfg.height, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000)))
		panic("read_screen");

	SDL_Rect bnds = {0, 0, gfx_cfg.width, gfx_cfg.height};
	SDL_RenderReadPixels(renderer, &bnds, SDL_PIXELFORMAT_ARGB8888, capture->pixels, capture->pitch);

	if (!(tex = SDL_CreateTextureFromSurface(renderer, capture)))
		panic("read_screen");
}

void Renderer::dump_screen() {
	SDL_Rect pos = {0, 0, gfx_cfg.width, gfx_cfg.height};
	SDL_RenderCopy(renderer, tex, NULL, &pos);
}

void Renderer::clear() {
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(renderer);
}

Renderer canvas;

static bool point_in_rect(int x, int y, int w, int h, int px, int py)
{
	return px >= x && px < x + w && py >= y && py < y + h;
}

/**
 * Core User Interface element.
 * This is the minimum interface for anything User Interface related
 * (e.g. text, buttons)
 */
class UI {
public:
	int x, y;
	unsigned w, h;

	UI(int x, int y, unsigned w=1, unsigned h=1)
		: x(x), y(y), w(w), h(h) {}
	virtual ~UI() {}

	virtual void draw() const = 0;

	bool contains(int px, int py) {
		return point_in_rect(x, y, w, h, px, py);
	}

	virtual void reshape(int x, int y, unsigned w, unsigned h) {
		this->x = x; this->y = y; this->w = w; this->h = h;
	}
};

class UI_Clickable {
protected:
	UI_Clickable() {}
public:
	virtual bool mousedown(SDL_MouseButtonEvent *event) = 0;
	virtual bool mouseup(SDL_MouseButtonEvent *event) = 0;
};

class UI_Container : public UI {
public:
	UI_Container(int x, int y, unsigned w, unsigned h)
		: UI(x, y, w, h) {}

	virtual ~UI_Container() {}
};

// FIXME grab these from the color palette
const SDL_Color col_default = {255, 255, 255, SDL_ALPHA_OPAQUE};

// FIXME grab these from the game palette
const SDL_Color col_players[MAX_PLAYER_COUNT] = {
	{0, 0, 196, SDL_ALPHA_OPAQUE},
	{200, 0, 0, SDL_ALPHA_OPAQUE},
	{234, 234, 0, SDL_ALPHA_OPAQUE},
	{140, 90, 33, SDL_ALPHA_OPAQUE},
	{255, 128, 0, SDL_ALPHA_OPAQUE},
	{0, 128, 0, SDL_ALPHA_OPAQUE},
	{128, 128, 128, SDL_ALPHA_OPAQUE},
	{64, 128, 128, SDL_ALPHA_OPAQUE},
};

class Text final : public UI {
public:
	const std::string str;
private:
	SDL_Surface *surf;
	SDL_Texture *tex;

public:
	Text(int x, int y, unsigned id
		, TextAlign halign=LEFT
		, TextAlign valign=TOP
		, TTF_Font *fnt=fnt_default)
		: UI(x, y), str(load_string(id))
	{
		SDL_Color col = {255, 255, 255, SDL_ALPHA_OPAQUE};
		surf = TTF_RenderText_Solid(fnt, str.c_str(), col);
		tex = SDL_CreateTextureFromSurface(renderer, surf);

		reshape(halign, valign);
	}

	Text(int x, int y, const std::string &str
		, TextAlign halign=LEFT
		, TextAlign valign=TOP
		, TTF_Font *fnt=fnt_default
		, SDL_Color col=col_default)
		: UI(x, y), str(str)
	{
		surf = TTF_RenderText_Solid(fnt, str.c_str(), col);
		tex = SDL_CreateTextureFromSurface(renderer, surf);

		reshape(halign, valign);
	}

	Text(int x, int y, const char *str
		, TextAlign halign=LEFT
		, TextAlign valign=TOP
		, TTF_Font *fnt=fnt_default
		, SDL_Color col=col_default)
		: UI(x, y), str(str)
	{
		surf = TTF_RenderText_Solid(fnt, str, col);
		tex = SDL_CreateTextureFromSurface(renderer, surf);

		reshape(halign, valign);
	}

	~Text() {
		SDL_DestroyTexture(tex);
		SDL_FreeSurface(surf);
	}

private:
	void reshape(TextAlign halign, TextAlign valign) {
		this->w = surf->w;
		this->h = surf->h;

		switch (halign) {
		case LEFT: break;
		case CENTER: this->x -= (int)w / 2; break;
		case RIGHT: this->x -= (int)w; break;
		}

		switch (valign) {
		case TOP: break;
		case MIDDLE: this->y -= (int)h / 2; break;
		case BOTTOM: this->y -= (int)h; break;
		}
	}

public:
	void move(int x, int y, TextAlign halign, TextAlign valign) {
		this->x = x;
		this->y = y;

		reshape(halign, valign);
	}

	void draw() const override {
		draw(false);
	}

	void draw(bool focus) const {
		draw(x, y, focus);
	}

	void draw(int x, int y, bool focus=false) const {
		SDL_Rect pos = {x - 1, y + 1, (int)w, (int)h};
		// draw shadow
		SDL_SetTextureColorMod(tex, 0, 0, 0);
		SDL_RenderCopy(renderer, tex, NULL, &pos);
		++pos.x;
		--pos.y;
		// draw foreground text
		RendererState &s = canvas.get_state();
		SDL_Color &col = focus ? s.col_text_f : s.col_text;
		SDL_SetTextureColorMod(tex, col.r, col.g, col.b);
		SDL_RenderCopy(renderer, tex, NULL, &pos);
	}
};

void Renderer::draw_text(int x, int y, const char *str
	, TextAlign halign, TextAlign valign, TTF_Font *fnt)
{
	Text text(x, y, str, halign, valign, fnt);
	text.draw();
}

void Renderer::draw_text(int x, int y, unsigned id
	, TextAlign halign, TextAlign valign, TTF_Font *fnt)
{
	Text text(x, y, id, halign, valign, fnt);
	text.draw();
}

SDL_Color border_cols[6] = {};

class Border : public UI {
	bool fill, invert;
	int shade;
public:
	Border(int x, int y, unsigned w=1, unsigned h=1, bool fill=true, bool invert=false)
		: UI(x, y, w, h), fill(fill), invert(invert), shade(-1) {}

	Border(int x, int y, unsigned w, unsigned h, int shade)
		: UI(x, y, w, h), fill(true), invert(false), shade(shade) {}

	void draw() const {
		draw(invert);
	}

	void draw(bool invert) const {
		unsigned w = this->w - 1, h = this->h - 1;

		int table[] = {0, 1, 2, 3, 4, 5}, table_r[] = {1, 0, 3, 2, 5, 4};
		int *colptr = invert ? table_r : table;

		// Draw background
		if (fill) {
			SDL_BlendMode old;
			SDL_GetRenderDrawBlendMode(renderer, &old);

			SDL_Rect pos = {x, y, (int)w, (int)h};
			SDL_Color col = {0, 0, 0, (Uint8)((shade != -1 ? shade : canvas.shade) * 255 / 100)};
			canvas.col(col);
			SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
			SDL_RenderFillRect(renderer, &pos);

			SDL_SetRenderDrawBlendMode(renderer, old);
		}

		// Draw outermost lines
		canvas.col(border_cols[colptr[0]]);
		SDL_RenderDrawLine(renderer, x, y    , x    , y + h);
		SDL_RenderDrawLine(renderer, x, y + h, x + w, y + h);
		canvas.col(border_cols[colptr[1]]);
		SDL_RenderDrawLine(renderer, x + 1, y, x + w, y        );
		SDL_RenderDrawLine(renderer, x + w, y, x + w, y + h - 1);
		// Draw middle lines
		canvas.col(border_cols[colptr[2]]);
		SDL_RenderDrawLine(renderer, x + 1, y + 1    , x + 1    , y + h - 1);
		SDL_RenderDrawLine(renderer, x + 1, y + h - 1, x + w - 1, y + h - 1);
		canvas.col(border_cols[colptr[3]]);
		SDL_RenderDrawLine(renderer, x + 2    , y + 1, x + w - 1, y + 1    );
		SDL_RenderDrawLine(renderer, x + w - 1, y + 1, x + w - 1, y + h - 2);
		// Draw innermost lines
		canvas.col(border_cols[colptr[4]]);
		SDL_RenderDrawLine(renderer, x + 2, y + 2    , x + 2    , y + h - 2);
		SDL_RenderDrawLine(renderer, x + 2, y + h - 2, x + w - 2, y + h - 2);
		canvas.col(border_cols[colptr[5]]);
		SDL_RenderDrawLine(renderer, x + 3    , y + 2, x + w - 2, y + 2    );
		SDL_RenderDrawLine(renderer, x + w - 2, y + 2, x + w - 2, y + h - 3);
	}
};

void Palette::open(unsigned id) {
	size_t n;
	const char *data = (const char*)drs_get_item(DT_BINARY, id, &n);

	unsigned colors;
	int offset;

	if (sscanf(data,
		"JASC-PAL\n"
		"%*s\n"
		"%u%n\n", &colors, &offset) != 1)
	{
		panicf("Bad palette id %u\n", id);
	}

	if (colors > 256)
		panicf("Bad palette id %u: too many colors", id);

	if (!(pal = SDL_AllocPalette(colors)))
		panic("Out of memory");

	for (unsigned i = 0; i < colors; ++i) {
		int n;
		unsigned cols[3];
		SDL_Color col;

		if (sscanf(data + offset,
			"%u %u %u%n\n", &cols[0], &cols[1], &cols[2], &n) != 3 || n < 0)
			goto fail;

		col.r = cols[0];
		col.g = cols[1];
		col.b = cols[2];
		col.a = SDL_ALPHA_OPAQUE;

		if (SDL_SetPaletteColors(pal, &col, i, 1))
fail:
			panicf("Bad palette id %u: bad color %u\n", id, i);
		offset += n;
	}
}

void Palette::set_border_color(unsigned id, unsigned col_id) const {
	border_cols[id].r = pal->colors[col_id].r;
	border_cols[id].g = pal->colors[col_id].g;
	border_cols[id].b = pal->colors[col_id].b;
	border_cols[id].a = SDL_ALPHA_OPAQUE;
}

Image::Image() : surface(NULL), texture(NULL) {}

bool Image::load(Palette *pal, const void *data, const struct slp_frame_info *frame, unsigned player)
{
	bool dynamic;

	hotspot_x = frame->hotspot_x;
	hotspot_y = frame->hotspot_y;

	dbgf("player: %u\n", player);

	dbgf("dimensions: %u x %u\n", frame->width, frame->height);
	dbgf("hotspot: %u,%u\n", frame->hotspot_x, frame->hotspot_y);
	dbgf("command table offset: %X\n", frame->cmd_table_offset);
	dbgf("outline table offset: %X\n", frame->outline_table_offset);

	// FIXME big endian support
	surface = std::shared_ptr<SDL_Surface>(
		SDL_CreateRGBSurface(0, frame->width, frame->height, 8, 0, 0, 0, 0),
		SDL_FreeSurface
	);

	dynamic = slp_read(surface.get(), pal->pal, data, frame, player);

	if (!(texture = std::shared_ptr<SDL_Texture>(SDL_CreateTextureFromSurface(renderer, surface.get()), SDL_DestroyTexture)))
		panicf("Cannot create texture: %s\n", SDL_GetError());

	return dynamic;
}

void Image::draw(int x, int y, unsigned w, unsigned h) const {
	canvas.draw(texture.get(), surface.get(), x - hotspot_x, y - hotspot_y, w, h);
}

void AnimationTexture::open(Palette *pal, unsigned id) {
	size_t n;
	const void *data = drs_get_item(DT_SLP, id, &n);

	this->id = id;
	slp_map(&image, data);

	if (memcmp(image.hdr->version, "2.0N", 4))
		panicf("Bad Animation Texture id %u", id);

	dbgf("slp info %u:\n", id);
	dbgf("frame count: %u\n", image.hdr->frame_count);

	images.reset(new Image[image.hdr->frame_count]);

	dynamic = false;

	// parse all frames
	for (size_t i = 0, n = image.hdr->frame_count; i < n; ++i) {
		dbgf("frame %zu:\n", i);
		struct slp_frame_info *frame = &image.info[i];
		if (images[i].load(pal, data, frame)) {
			dynamic = true;
			break;
		}
	}

	// use custom parsing if dynamic
	if (!dynamic)
		return;

	dbgf("AnimationTexture::open: dynamic id %u\n", id);

	images.reset(new Image[image.hdr->frame_count * MAX_PLAYER_COUNT]);

	// parse all dynamic frames
	for (size_t p = 0; p < MAX_PLAYER_COUNT; ++p)
		for (size_t i = 0, n = image.hdr->frame_count; i < n; ++i) {
			struct slp_frame_info *frame = &image.info[i];
			images[p * n + i].load(pal, data, frame, p);
		}
}

void AnimationTexture::draw(int x, int y, unsigned index, unsigned player) const {
	if (!dynamic)
		player = 0;
	if (player > 7)
		player = 7;

	size_t n = image.hdr->frame_count;

	images[player * n + index % n].draw(x, y);
}

void AnimationTexture::draw(int x, int y, unsigned w, unsigned h, unsigned index, unsigned player) const {
	if (!dynamic)
		player = 0;
	if (player > 7)
		player = 7;

	size_t n = image.hdr->frame_count;

	//dbgf("draw (%d,%d) (%u,%u)\n", x, y, w, h);
	images[player * n + index % n].draw(x, y, w, h);
}

void AnimationTexture::draw_selection(int x, int y, unsigned size) const {
	canvas.draw_selection(x, y, size);
}

class Background final : public UI {
	unsigned id;
	Palette palette;
	AnimationTexture animation;
	unsigned bevel_col[6];
public:
	Background(unsigned id, int x, int y, unsigned w = 800, unsigned h = 600)
		: UI(x, y, w, h), id(id), palette(), animation()
	{
		size_t n;
		const char *data = (const char*)drs_get_item(DT_BINARY, id, &n);

		char bkg_name1[16], bkg_name2[16], bkg_name3[16];
		int bkg_id[3];
		char pal_name[16];
		unsigned pal_id;
		char cur_name[16];
		unsigned cur_id;
		char btn_file[16];
		int btn_id;

		char dlg_name[16];
		unsigned dlg_id;
		unsigned bkg_pos, bkg_col;
		unsigned text_col[6];
		unsigned focus_col[6];
		unsigned state_col[6];

		if (sscanf(data,
			"background1_files %15s none %d -1\n"
			"background2_files %15s none %d -1\n"
			"background3_files %15s none %d -1\n"
			"palette_file %15s %u\n"
			"cursor_file %15s %u\n"
			"shade_amount percent %u\n"
			"button_file %s %d\n"
			"popup_dialog_sin %15s %u\n"
			"background_position %u\n"
			"background_color %u\n"
			"bevel_colors %u %u %u %u %u %u\n"
			"text_color1 %u %u %u\n"
			"text_color2 %u %u %u\n"
			"focus_color1 %u %u %u\n"
			"focus_color2 %u %u %u\n"
			"state_color1 %u %u %u\n"
			"state_color2 %u %u %u\n",
			bkg_name1, &bkg_id[0],
			bkg_name2, &bkg_id[1],
			bkg_name3, &bkg_id[2],
			pal_name, &pal_id,
			cur_name, &cur_id,
			&canvas.shade,
			btn_file, &btn_id,
			dlg_name, &dlg_id,
			&bkg_pos, &bkg_col,
			&bevel_col[0], &bevel_col[1], &bevel_col[2],
			&bevel_col[3], &bevel_col[4], &bevel_col[5],
			&text_col[0], &text_col[1], &text_col[2],
			&text_col[3], &text_col[4], &text_col[5],
			&focus_col[0], &focus_col[1], &focus_col[2],
			&focus_col[3], &focus_col[4], &focus_col[5],
			&state_col[0], &state_col[1], &state_col[2],
			&state_col[3], &state_col[4], &state_col[5]) != 6 + 11 + 4 * 6)
		{
			panicf("Bad background: id = %u", id);
		}

		palette.open(pal_id);
		// Apply palette to border colors
		restore();

		// dialogs may have -1 as background index to indicate to always use the first one
		if (bkg_id[1] < 0)
			bkg_id[1] = bkg_id[0];
		if (bkg_id[2] < 0)
			bkg_id[2] = bkg_id[0];

		unsigned which;

		switch (cfg.screen_mode) {
		case CFG_MODE_640x480: which = 0; break;
		case CFG_MODE_1024x768: which = 2; break;
		default: which = 1; break;
		}

		animation.open(&palette, (unsigned)bkg_id[which]);
	}

	void restore() {
		palette.set_border_color(0, bevel_col[5]);
		palette.set_border_color(1, bevel_col[0]);
		palette.set_border_color(2, bevel_col[4]);
		palette.set_border_color(3, bevel_col[1]);
		palette.set_border_color(4, bevel_col[3]);
		palette.set_border_color(5, bevel_col[2]);
	}

	void draw() const {
		animation.draw(x, y, w, h, 0);
	}
};

static constexpr int text_halign(TextAlign align, int w, int margin=4)
{
	return align == LEFT ? margin : align == CENTER ? w / 2 : w - margin;
}

static constexpr int text_valign(TextAlign align, int h, int margin=4)
{
	return align == TOP ? margin : align == MIDDLE ? h / 2 : h - margin;
}

class Button final : public Border, public UI_Clickable {
	Text text;
public:
	bool focus;
	bool down, hold;
	bool play_sfx;
	bool visible;
	unsigned sfx;

	Button(int x, int y, unsigned w, unsigned h, unsigned id, bool def_fnt=false, bool play_sfx=true, unsigned sfx=SFX_BUTTON4, bool hold=false, TextAlign halign=CENTER, TextAlign valign=MIDDLE)
		: Border(x, y, w, h)
		, text(x + text_halign(halign, w), y + text_valign(valign, h), id, halign, valign, def_fnt ? fnt_default : fnt_button)
		, focus(false), down(false), hold(hold), play_sfx(play_sfx), visible(true), sfx(sfx)
	{
	}

	Button(int x, int y, unsigned w, unsigned h, const std::string &str, bool def_fnt=false, bool play_sfx=true, unsigned sfx=SFX_BUTTON4, bool hold=false, TextAlign halign=CENTER, TextAlign valign=MIDDLE, SDL_Color col=col_default)
		: Border(x, y, w, h)
		, text(x + w / 2, y + h / 2, str, halign, valign, def_fnt ? fnt_default : fnt_button, col)
		, focus(false), down(false), hold(hold), play_sfx(play_sfx), visible(true), sfx(sfx)
	{
	}

	/* Process mouse event if it has been clicked. */
	bool mousedown(SDL_MouseButtonEvent *event) override {
		bool old_down = down;

		focus = down = visible && contains(event->x, event->y);

		if (old_down != down)
			ui_state.dirty();

		return down;
	}

	/* Process mouse event if it has been clicked. */
	bool mouseup(SDL_MouseButtonEvent *event) override {
		if (down) {
			down = false;
			ui_state.dirty();

			bool hit = contains(event->x, event->y);
			if (hit && play_sfx)
				sfx_play(sfx);
			return hit;
		}

		// Button wasn't pressed so we can't munge the event.
		return false;
	}

	void draw() const {
		if (!visible)
			return;
		Border::draw(hold || down);
		text.draw(hold || focus);
	}
};

class VerticalSlider : public UI_Container, public UI_Clickable {
	Border b;
	Button up, down, middle;
public:
	float value;
	float step;

	static constexpr unsigned margin = 2;

	VerticalSlider(int x, int y, unsigned w, unsigned h, float value, float step=0.1)
		: UI_Container(x, y, w, h)
		, b(x, y, w, h)
		, up(x + margin, y + margin, w - 2 * margin, w - 2 * margin, "^")
		, down(x + margin, y + h - w + margin, w - 2 * margin, w - 2 * margin, "v")
		, middle(x + margin, y + w - margin + (int)((1.0f - value) * (h - 2 * (w - margin) - (w - 2 * margin))), w - 2 * margin, w - 2 * margin, " ")
		, value(value), step(step)
	{
	}

	bool mousedown(SDL_MouseButtonEvent *event) override {
		if (!contains(event->x, event->y))
			return false;

		return up.mousedown(event) || down.mousedown(event) || middle.mousedown(event);
	}

	bool mouseup(SDL_MouseButtonEvent *event) override {
		if (!contains(event->x, event->y))
			return false;

		if (up.mouseup(event)) {
			set(value + step);
			return true;
		}

		if (down.mouseup(event)) {
			set(value - step);
			return true;
		}

		if (middle.mouseup(event))
			return true;

		set(1.0f - (event->y - (y + w - margin - (w - 2 * margin) / 2)) / (float)(h - w + margin - (w - 2 * margin)));
		return true;
	}

	void draw() const override {
		b.draw();
		up.draw();
		down.draw();
		middle.draw();
	}

	void set(float value) {
		this->value = saturate(0.0f, value, 1.0f);
		middle.y = y + w - margin + (int)((1.0f - this->value) * (h - 2 * (w - margin) - (w - 2 * margin)));
	}
};

// TODO create popup selector for folded non-list
class SelectorArea : public Border, public UI_Clickable {
	Text hdr;
	std::vector<std::unique_ptr<Text>> items;
	unsigned selected, start;
	Border box_sel;
	unsigned orig_w, max;
	bool fold, list; // list indicates whether scroll is visible
	// visible when area is too tight (i.e. folded)
	Button btn_open;
	VerticalSlider scroll;
	static constexpr unsigned margin = 2;
	static constexpr unsigned vspace = 21;
	static constexpr unsigned btn_open_width = 22;
public:
	SelectorArea(int x, int y, unsigned w, unsigned h, unsigned id, int offx=5, int offy=-20, int shade=-1)
		: Border(x, y, w, h, shade)
		, hdr(x + offx, y + offy, id, LEFT, TOP, fnt_button)
		, items(), selected(0), start(0)
		, box_sel(x + margin, y + margin, w - 2 * margin, 19 + 2 * margin)
		, orig_w(w), max(0), fold(false), list(false)
		, btn_open(x + w - btn_open_width, y, btn_open_width, h, "v")
		, scroll(x + w - btn_open_width, y, btn_open_width, h, 1)
	{
		assert(h >= 2 * margin);
		btn_open.visible = false;
	}

	bool mousedown(SDL_MouseButtonEvent *event) override {
		unsigned w = fold ? orig_w - btn_open_width : orig_w;
		if (point_in_rect(x + margin, y, w - 2 * margin, h, event->x, event->y)) {
			int rel_y = (event->y - (y + 2 * margin)) / vspace;
			if (rel_y >= 0 && rel_y < (int)items.size()) {
				select(rel_y);
				return true;
			}
			return false;
		}

		if (!fold)
			return false;

		return list ? scroll.mousedown(event) : btn_open.mousedown(event);
	}

	bool mouseup(SDL_MouseButtonEvent *event) override {
		if (!fold)
			return false;
		if (!list) {
			if (btn_open.mouseup(event)) {
				selected = (selected + 1) % items.size();
				update();
				return true;
			}
			return false;
		}
		if (scroll.mouseup(event)) {
			update();
			return true;
		}
		return false;
	}

private:
	void update_scroll() {
		if (!fold || !list) {
			box_sel.y = y + margin + selected * vspace;
			return;
		}
		int end = (vspace * items.size() - h - 1) / vspace + 1;
		start = (unsigned)(end * (1.0f - scroll.value));
		box_sel.y = y + margin + (selected - start) * vspace;
	}

	void update() {
		fold = y + items.size() * vspace >= y + h - vspace - margin;
		if (fold) {
			list = h > 2 * vspace + 2 * margin;
			btn_open.visible = !list;
			if (list) {
				// compute maximum elements that fit in area
				max = (h - 2 * margin) / vspace;
			}
			box_sel.w = orig_w - 2 * margin - btn_open_width;
			this->w = orig_w - btn_open_width;
		} else {
			list = false;
			btn_open.visible = false;
			box_sel.w = orig_w - 2 * margin;
			this->w = orig_w;
		}
		update_scroll();
	}

public:
	void add(int id) {
		int y = this->y + items.size() * vspace;
		items.emplace_back(new Text(x + 2 * margin + 1, y + 2 * margin + 2, id));
		update();
	}

	void add(std::string text) {
		int y = this->y + items.size() * vspace;
		items.emplace_back(new Text(x + 2 * margin + 1, y + 2 * margin + 2, text));
		update();
	}

	void clear() {
		items.clear();
		selected = 0;
		scroll.set(1);
		update();
	}

	void select(unsigned index) {
		selected = start + index;
		update();
	}

	unsigned focus() const {
		return selected;
	}

	const std::string &focus_text() const {
		return items[selected]->str;
	}

	void draw() const {
		Border::draw(true);
		if (!fold)
			box_sel.draw();
		else if (list) {
			if (box_sel.y >= y + margin && box_sel.y <= (int)(y + h - vspace - margin))
				box_sel.draw();
			scroll.draw();
		}
		btn_open.draw();
		hdr.draw();

		if (fold) {
			if (list) {
				unsigned index = start, max = items.size();
				for (int y = this->y + 2 * margin + 2, bottom = this->y + h - vspace - 2 * margin; index < max && y < bottom; y += vspace, ++index) {
					Text *txt = items[index].get();
					txt->x = x + 2 * margin + 1;
					txt->y = y;
					txt->draw(index == selected);
				}
			} else {
				Text *txt = items[selected].get();
				txt->x = x + 2 * margin + 1;
				txt->y = y + 2 * margin + 2;
				txt->draw();
			}
		} else {
			unsigned i = 0;
			for (auto &x : items)
				x->draw(i++ == selected);
		}
	}
};

/* Provides a group of buttons the user also can navigate through with arrow keys */
class ButtonGroup final : public UI {
	std::vector<std::shared_ptr<Button>> objects;
	unsigned old_focus = 0;
	bool down_ = false;
	bool no_focus = false;

public:
	unsigned focus = 0;

	ButtonGroup(int x=212, int y=222, unsigned w=375, unsigned h=50)
		: UI(x, y, w, h), objects() {}

	void add(int rel_x, int rel_y, unsigned id, bool def_fnt) {
		add(rel_x, rel_y, id, 0, 0, def_fnt);
	}

	void add(int rel_x, int rel_y, unsigned id=STR_ERROR, unsigned w=0, unsigned h=0, bool def_fnt=false) {
		if (!w) w = this->w;
		if (!h) h = this->h;

		objects.emplace_back(new Button(x + rel_x, y + rel_y, w, h, id, def_fnt));
	}

	void update() {
		if (no_focus)
			return;
		auto old = objects[old_focus].get();
		auto next = objects[focus].get();

		old->focus = false;
		next->focus = true;
	}

	/* Rotate right through button group */
	void ror() {
		use_focus();
		if (down_)
			return;

		old_focus = focus;
		focus = (focus + 1) % objects.size();

		if (old_focus != focus)
			ui_state.dirty();
	}

	/* Rotate left through button group */
	void rol() {
		use_focus();
		if (down_)
			return;

		old_focus = focus;
		focus = (focus + objects.size() - 1) % objects.size();

		if (old_focus != focus)
			ui_state.dirty();
	}

	void mousedown(SDL_MouseButtonEvent *event) {
		unsigned id = 0;
		old_focus = focus;

		for (auto x : objects) {
			auto btn = x.get();

			if (btn->mousedown(event)) {
				use_focus();
				focus = id;
			} else
				btn->focus = false;

			++id;
		}
	}

	bool mouseup(SDL_MouseButtonEvent *event) {
		return objects[focus].get()->mouseup(event);
	}

	void down() {
		use_focus();
		down_ = true;
		objects[focus].get()->down = true;
		ui_state.dirty();
	}

	void up() {
		down_ = false;
		objects[focus].get()->down = false;
		ui_state.dirty();
	}

	void draw() const {
		for (auto x : objects)
			x.get()->draw();
	}

	void release_focus() {
		auto btn = objects[focus].get();
		btn->down = btn->focus = false;
		no_focus = true;
		ui_state.dirty();
	}

	void use_focus() {
		if (no_focus) {
			no_focus = false;
			ui_state.dirty();
		}
	}
};

/* Provides a group of buttons where exactly one is selected at any time */
class ButtonRadioGroup final : public UI_Container, public UI_Clickable {
	std::vector<std::shared_ptr<Button>> objects;
	std::vector<std::shared_ptr<Text>> labels;
	unsigned old_focus = 0;
	bool down_ = false;
	bool no_focus = false;
	bool custom_text;

public:
	unsigned focus = 0;

	ButtonRadioGroup(bool custom_text, int x=5, int y=5, unsigned w=30, unsigned h=30)
		: UI_Container(x, y, w, h), objects(), custom_text(custom_text) {}

	void add(int rel_x, int rel_y, unsigned id, bool def_fnt) {
		add(rel_x, rel_y, id, 0, 0, def_fnt);
	}

	void add(int rel_x, int rel_y, unsigned id, unsigned w, unsigned h, bool def_fnt=false, TextAlign halign=CENTER, TextAlign valign=MIDDLE, bool play_sfx=true) {
		if (!w) w = this->w;
		if (!h) h = this->h;

		if (custom_text) {
			objects.emplace_back(new Button(x + rel_x, y + rel_y, w, h, STR_EXIT, def_fnt, play_sfx, SFX_BUTTON4, objects.size() == 0, halign, valign));
			labels.emplace_back(new Text(x + rel_x + w + 10, y + rel_y + h / 2, id, LEFT, MIDDLE));
		} else {
			objects.emplace_back(new Button(x + rel_x, y + rel_y, w, h, id, def_fnt, play_sfx, SFX_BUTTON4, objects.size() == 0, halign, valign));
		}
	}

	void select(unsigned id) {
		assert(id < objects.size());
		objects[old_focus = focus].get()->hold = false;
		objects[focus = id].get()->hold = true;
	}

	bool mousedown(SDL_MouseButtonEvent *event) override {
		bool pressed = false;
		unsigned id = 0;
		old_focus = focus;

		for (auto x : objects) {
			auto btn = x.get();

			if (btn->mousedown(event)) {
				use_focus();
				focus = id;
				pressed = true;
			} else {
				btn->focus = false;
			}

			++id;
		}

		return pressed;
	}

	bool mouseup(SDL_MouseButtonEvent *event) override {
		unsigned id = 0;
		bool pressed = objects[focus].get()->mouseup(event);

		if (!pressed)
			return false;

		for (auto x : objects) {
			auto btn = x.get();
			btn->hold = id == focus;
			++id;
		}

		return true;
	}

	void draw() const {
		for (auto x : objects)
			x.get()->draw();
		for (auto l : labels)
			l.get()->draw();
	}

	void release_focus() {
		auto btn = objects[focus].get();
		btn->down = btn->focus = false;
		no_focus = true;
		ui_state.dirty();
	}

	void use_focus() {
		if (no_focus) {
			no_focus = false;
			ui_state.dirty();
		}
	}
};

class Menu : public UI {
protected:
	std::vector<std::shared_ptr<UI>> objects;
	mutable ButtonGroup group;
	Background *bkg;
public:
	int stop = 0;

	Menu(unsigned title_id, bool show_title=true, TTF_Font *fnt=fnt_large)
		: UI(0, 0, gfx_cfg.width, gfx_cfg.height), objects(), group(), bkg(nullptr)
	{
		if (show_title)
			objects.emplace_back(new Text(
				gfx_cfg.width / 2, 12, title_id, MIDDLE, TOP, fnt
			));
	}

	Menu(unsigned title_id, int x, int y, unsigned w, unsigned h, bool show_title=true, TTF_Font *fnt=fnt_large)
		: UI(0, 0, gfx_cfg.width, gfx_cfg.height), objects(), group(x, y, w, h), bkg(nullptr)
	{
		if (show_title)
			objects.emplace_back(new Text(
				gfx_cfg.width / 2, y + 12, title_id, MIDDLE, TOP, fnt
			));
	}

	virtual void idle() {}

	virtual void draw() const {
		for (auto x : objects)
			x.get()->draw();

		group.update();
		group.draw();
	}

	virtual void keydown(SDL_KeyboardEvent *event) {
		unsigned virt = event->keysym.sym;

		if (virt == SDLK_DOWN)
			group.ror();
		else if (virt == SDLK_UP)
			group.rol();
		else if (virt == ' ')
			group.down();
	}

	virtual void keyup(SDL_KeyboardEvent *event) {
		unsigned virt = event->keysym.sym;

		if (virt == ' ') {
			sfx_play(SFX_BUTTON4);
			group.up();
			button_group_activate(group.focus);
		}
	}

	virtual bool mousedown(SDL_MouseButtonEvent *event) {
		bool b = false;
		group.mousedown(event);

		for (auto x : objects) {
			UI_Clickable *btn = dynamic_cast<UI_Clickable*>(x.get());
			if (btn && btn->mousedown(event)) {
				group.release_focus();
				b = true;
			}
		}

		return b;
	}

	void mouseup(SDL_MouseButtonEvent *event) {
		if (group.mouseup(event)) {
			button_group_activate(group.focus);
			return;
		}

		unsigned id = 0;
		for (auto x : objects) {
			UI_Clickable *btn = dynamic_cast<UI_Clickable*>(x.get());
			if (btn && btn->mouseup(event))
				button_activate(id);
			++id;
		}
	}

	virtual void restore() {
		if (bkg)
			bkg->restore();
	}

	virtual void button_group_activate(unsigned id) = 0;

	virtual void button_activate(unsigned) {
		panic("Unimplemented");
	}
};

/*
NOTE regarding implementing playername Y-position in timeline:
the initial population for each player determines the Y-position for each playername

TODO compute and plot timeline progression for each player
*/

class MenuTimeline final : public Menu {
public:
	static unsigned constexpr tl_left = 37;
	static unsigned constexpr tl_right = 765;
	static unsigned constexpr tl_top = 151;
	static unsigned constexpr tl_bottom = 474;
	static unsigned constexpr tl_height = tl_bottom - tl_top;

	std::vector<std::shared_ptr<UI>> player_objects;

	MenuTimeline(unsigned type) : Menu(STR_TITLE_ACHIEVEMENTS, 0, 0, 550 - 250, 588 - 551, false) {
		objects.emplace_back(bkg = new Background(type ? type == 2 ? DRS_BACKGROUND_VICTORY : DRS_BACKGROUND_DEFEAT : DRS_BACKGROUND_ACHIEVEMENTS, 0, 0));
		objects.emplace_back(new Border(0, 0, gfx_cfg.width, gfx_cfg.height, false));
		objects.emplace_back(new Button(779, 4, 795 - 779, 16, STR_EXIT, true));
		objects.emplace_back(new Text(
			gfx_cfg.width / 2, 8, STR_TITLE_ACHIEVEMENTS, MIDDLE, TOP, fnt_large
		));

		objects.emplace_back(new Text(gfx_cfg.width / 2, 44, STR_BTN_TIMELINE, CENTER, TOP, fnt_large));

		// TODO compute elapsed time
		objects.emplace_back(new Text(685, 15, "00:00:00"));

		objects.emplace_back(new Border(12, 106, 787 - 12, 518 - 106, 100));

		group.add(250, 551, STR_BTN_BACK);

		unsigned i = 1, step = tl_height / game.player_count();
		TTF_SetFontStyle(fnt_default, TTF_STYLE_BOLD);

		for (auto p : game.players) {
			char buf[64];
			char civbuf[64];
			load_string(&lib_lang, STR_CIV_EGYPTIAN + p->civ, civbuf, sizeof civbuf);
			snprintf(buf, sizeof buf, "%s - %s", p->name.c_str(), civbuf);

			player_objects.emplace_back(
				new Text(
					40, tl_top + i * step - step / 2,
					buf, LEFT, MIDDLE
				)
			);
			++i;
		}

		TTF_SetFontStyle(fnt_default, TTF_STYLE_NORMAL);
		canvas.clear();
	}

	virtual void button_group_activate(unsigned) override final {
		stop = 1;
	}

	void button_activate(unsigned id) override final {
		switch (id) {
		case 3: running = 0; break;
		}
	}

	virtual void draw() const override {
		Menu::draw();

#ifndef _WIN32
		unsigned i, n = game.player_count(), step = (tl_height + 1) / n;
		for (i = 0; i < n; ++i) {
			const SDL_Color *col = &col_players[i];

			Sint16 xx[4], yy[4];

			xx[0] = tl_left; yy[0] = tl_top + i * step;
			xx[1] = xx[0]; yy[1] = tl_top + (i + 1) * step;
			xx[2] = tl_right; yy[2] = yy[1];
			xx[3] = xx[2]; yy[3] = yy[0];

			filledPolygonRGBA(
				renderer,
				xx, yy, 4, col->r, col->g, col->b, SDL_ALPHA_OPAQUE
			);
		}
#endif

		canvas.col(255);
		SDL_RenderDrawLine(renderer, tl_left, tl_top, tl_right, tl_top);
		SDL_RenderDrawLine(renderer, tl_right, tl_top, tl_right, tl_bottom);
		SDL_RenderDrawLine(renderer, tl_left, tl_top, tl_left, tl_bottom);
		SDL_RenderDrawLine(renderer, tl_left, tl_bottom, tl_right, tl_bottom);

		for (auto x : player_objects)
			x.get()->draw();
	}
};

class MenuAchievements final : public Menu {
	unsigned type;
	ButtonRadioGroup *cat;
public:
	MenuAchievements(unsigned type = 0) : Menu(STR_TITLE_ACHIEVEMENTS, 0, 0, type ? 775 - 550 : 375 - 125, 588 - 551, false), type(type) {
		objects.emplace_back(bkg = new Background(type ? type == 2 ? DRS_BACKGROUND_VICTORY : DRS_BACKGROUND_DEFEAT : DRS_BACKGROUND_ACHIEVEMENTS, 0, 0));
		objects.emplace_back(new Border(0, 0, gfx_cfg.width, gfx_cfg.height, false));
		objects.emplace_back(new Button(779, 4, 795 - 779, 16, STR_EXIT, true));
		objects.emplace_back(new Text(
			gfx_cfg.width / 2, 8, STR_TITLE_ACHIEVEMENTS, MIDDLE, TOP, fnt_large
		));

		objects.emplace_back(new Text(gfx_cfg.width / 2, 44, STR_TITLE_SUMMARY, CENTER, TOP, fnt_large));

		// TODO compute elapsed time
		objects.emplace_back(new Text(685, 15, "00:00:00"));

		if (type) {
			group.add(550, 551, STR_BTN_CLOSE);
			group.add(25, 551, STR_BTN_TIMELINE);
			group.add(287, 551, STR_BTN_RESTART);
		} else {
			group.add(425, 551, STR_BTN_CLOSE);
			group.add(125, 551, STR_BTN_TIMELINE);
		}

		// TODO use radiobuttongroup
		cat = new ButtonRadioGroup(false, 0, 0, 289 - 124, 212 - 187);
		cat->add(124, 187, STR_BTN_MILITARY, 289 - 124, 212 - 187, true, LEFT, TOP, false);
		cat->add(191, 162, STR_BTN_ECONOMY, 354 - 191, 187 - 162, true, LEFT, TOP, false);
		cat->add(266, 137, STR_BTN_RELIGION, 420 - 266, 162 - 138, true, LEFT, TOP, false);
		cat->add(351, 112, STR_BTN_TECHNOLOGY, 549 - 351, 137 - 112, true, MIDDLE, TOP, false);
		objects.emplace_back(cat);

		objects.emplace_back(new Text(633, 140, STR_SURVIVAL, RIGHT, TOP, fnt_default));
		objects.emplace_back(new Text(710, 165, STR_WONDER, RIGHT, TOP, fnt_default));
		objects.emplace_back(new Text(778, 190, STR_TOTAL_SCORE, RIGHT, TOP, fnt_default));

		unsigned y = 225, nr = 0;

		TTF_SetFontStyle(fnt_default, TTF_STYLE_BOLD);

		for (auto p : game.players) {
			objects.emplace_back(new Text(31, y, p->name, LEFT, TOP, fnt_default, col_players[nr % MAX_PLAYER_COUNT]));

			y += 263 - 225;
			++nr;
		}

		TTF_SetFontStyle(fnt_default, TTF_STYLE_NORMAL);

		if (type)
			mus_play(MUS_DEFEAT);
	}

	virtual void button_group_activate(unsigned id) override final {
		switch (id) {
		case 0:
			if (type) {
				stop = 5;
				game.stop();
				mus_play(MUS_MAIN);
			} else
				stop = 1;
			break;
		case 1: ui_state.go_to(new MenuTimeline(type)); break;
		}
	}

	void button_activate(unsigned id) override final {
		switch (id) {
		case 2:
			running = 0;
			game.stop();
			break;
		}
	}
};

class MenuGameSettings final : public Menu {
	ButtonRadioGroup *gs, *ss, *mi, *tt, *pf;
	VerticalSlider *mv, *sv, *scroll;
public:
	MenuGameSettings() : Menu(STR_TITLE_GAME_SETTINGS, 100, 105, 700 - 100, 495 - 105, false) {
		const Player *you = game.controlling_player();

		objects.emplace_back(
			new Background(
				DRS_BACKGROUND_GAME_0 + menu_bar_tbl[you->civ],
				100, 105, 700 - 100, 495 - 105
			)
		);

		objects.emplace_back(new Border(100, 105, 700 - 100, 495 - 105, false));

		// add title manually because we want it to be drawn on top of the background
		objects.emplace_back(new Text(gfx_cfg.width / 2, 105 + 12, STR_TITLE_GAME_SETTINGS, MIDDLE, TOP, fnt_large));

		group.add(220 - 100, 450 - 105, STR_BTN_OK, 390 - 220, 480 - 450);
		group.add(410 - 100, 450 - 105, STR_BTN_CANCEL, 580 - 410, 480 - 450);

		objects.emplace_back(new Text(125, 163, STR_TITLE_SPEED));
		gs = new ButtonRadioGroup(true, 120, 190);
		gs->add(0, 0, STR_BTN_SPEED_NORMAL, true);
		gs->add(0, 225 - 190, STR_BTN_SPEED_FAST, true);
		gs->add(0, 260 - 190, STR_BTN_SPEED_HYPER, true);
		gs->select(game.speed);
		objects.emplace_back(gs);

		objects.emplace_back(new Text(270, 154, STR_TITLE_MUSIC));
		objects.emplace_back(mv = new VerticalSlider(270, 190, 20, 100, cfg.music_volume));

		objects.emplace_back(new Text(410, 154, STR_TITLE_SOUND));
		objects.emplace_back(sv = new VerticalSlider(410, 190, 20, 100, cfg.sound_volume));
		objects.emplace_back(new Text(550, 154, STR_TITLE_SCROLL));
		objects.emplace_back(scroll = new VerticalSlider(550, 190, 20, 100, cfg.scroll_speed));

		objects.emplace_back(new Text(125, 303, STR_TITLE_SCREEN));
		ss = new ButtonRadioGroup(true, 120, 330);
		ss->add(0, 0, STR_BTN_SCREEN_SMALL, true);
		ss->add(0, 365 - 330, STR_BTN_SCREEN_NORMAL, true);
		ss->add(0, 400 - 330, STR_BTN_SCREEN_LARGE, true);
		ss->select(cfg.screen_mode);
		objects.emplace_back(ss);

		objects.emplace_back(new Text(275, 303, STR_TITLE_MOUSE));
		mi = new ButtonRadioGroup(true, 270, 330);
		mi->add(0, 0, STR_BTN_MOUSE_NORMAL, true);
		mi->add(0, 365 - 330, STR_BTN_MOUSE_ONE, true);
		mi->select(!(cfg.options & CFG_NORMAL_MOUSE));
		objects.emplace_back(mi);

		objects.emplace_back(new Text(435, 303, STR_TITLE_HELP));
		tt = new ButtonRadioGroup(true, 430, 330);
		tt->add(0, 0, STR_BTN_HELP_ON, true);
		tt->add(0, 365 - 330, STR_BTN_HELP_OFF, true);
		tt->select(!(cfg.options & CFG_GAME_HELP));
		objects.emplace_back(tt);

		objects.emplace_back(new Text(565, 303, STR_TITLE_PATH));
	}

	void button_group_activate(unsigned id) override final {
		stop = 1;

		// if OK
		if (id == 0) {
			game.speed = gs->focus;
			cfg.screen_mode = ss->focus;
			if (mi->focus)
				cfg.options |= CFG_NORMAL_MOUSE;
			else
				cfg.options &= ~CFG_NORMAL_MOUSE;
			if (tt->focus)
				cfg.options &= ~CFG_GAME_HELP;
			else
				cfg.options |= CFG_GAME_HELP;
			cfg.music_volume = mv->value;
			cfg.sound_volume = sv->value;
			cfg.scroll_speed = scroll->value;
		}
	}

	void button_activate(unsigned id) override final {
		switch (id) {
		case 4:
		case 6:
		case 8:
		case 10:
		case 12:
		case 14:
		case 16:
			break;
		default:
			dbgf("game settings: id=%u\n", id);
			break;
		}
	}

	void draw() const override {
		canvas.dump_screen();
		Menu::draw();
	}
};

void walk_file_item(void *arg, char *name);

class MenuFileSelection final : public Menu {
	SelectorArea *sel_file;
	std::vector<std::string> files;
	bool strip;
public:
	MenuFileSelection(unsigned id, unsigned bkg_id, const char *dir, const char *ext, unsigned options, bool strip=true) : Menu(id, 0, 0, gfx_cfg.width, gfx_cfg.height), strip(strip) {
		char buf[FS_BUFSZ];
		int err;

		objects.emplace_back(bkg = new Background(bkg_id, 0, 0));
		objects.emplace_back(new Border(0, 0, gfx_cfg.width, gfx_cfg.height, false));
		objects.emplace_back(new Text(
			gfx_cfg.width / 2, 12, id, MIDDLE, TOP, fnt_button
		));
		objects.emplace_back(sel_file = new SelectorArea(25, 81, 775 - 21, 521 - 81, STR_TITLE_PATH));

		group.add(37, 550, STR_BTN_OK, 262 - 37, 587 - 550);
		group.add(287, 550, STR_BTN_DELETE, 512 - 287, 587 - 550);
		group.add(537, 550, STR_BTN_CANCEL, 762 - 537, 587 - 550);

		if ((err = fs_walk_ext(dir, ext, walk_file_item, this, buf, sizeof buf, options))) {
			show_error_code(err);
			stop = 1;
			return;
		}

		std::sort(files.begin(), files.end(), [](const std::string &a, const std::string &b) {
			return strcasecmp(a.c_str(), b.c_str()) < 0;
		});

		for (auto &file : files) {
			char buf[256];
			strncpy(buf, file.c_str(), sizeof buf);
			buf[(sizeof buf) - 1] = '\0';

			char *name = buf, *start = strrchr(name, '/');
			if (start)
				name = start + 1;

			if (strip) {
				char *ext = strrchr(name, '.');
				if (ext)
					*ext = '\0';
			}

			sel_file->add(name);
		}
	}

	void add_item(char *name) {
		files.emplace_back(name);
	}

	void button_activate(unsigned) override final {}

	void button_group_activate(unsigned id) override final {
		switch (id) {
		case 0:
			ui_state.file_selected = files[sel_file->focus()];
			stop = 1;
			break;
		case 2:
			ui_state.file_selected = "";
			stop = 1;
			break;
		}
		dbgf("selected: %s\n", ui_state.file_selected.c_str());
	}
};

void walk_file_item(void *arg, char *name)
{
	((MenuFileSelection*)arg)->add_item(name);
}

class MenuScenarioEditorMenu final : public Menu {
public:
	MenuScenarioEditorMenu() : Menu(STR_TITLE_MAIN, 200, 120, 600 - 200, 160 - 120, false) {
		objects.emplace_back(
			new Background(
				DRS_BACKGROUND_SCENARIO_EDITOR,
				170, 100, 630 - 170, 500 - 100
			)
		);
		objects.emplace_back(new Border(170, 100, 630 - 170, 500 - 100, false));

		group.add(0, 0, STR_BTN_SCENARIO_MENU_QUIT);
		group.add(0, 170 - 120, STR_BTN_SCENARIO_MENU_SAVE);
		group.add(0, 220 - 120, STR_BTN_SCENARIO_MENU_SAVE_AS);
		group.add(0, 270 - 120, STR_BTN_SCENARIO_MENU_EDIT);
		group.add(0, 320 - 120, STR_BTN_SCENARIO_CREATE);
		group.add(0, 370 - 120, STR_BTN_SCENARIO_MENU_TEST);
		group.add(0, 440 - 120, STR_BTN_SCENARIO_MENU_CANCEL);
	}

	void button_group_activate(unsigned id) override final {
		switch (id) {
		case 0:
			editor.stop();
			stop = 3;
			break;
		case 6:
			stop = 1;
			break;
		}
	};
};

class MenuGameMenu final : public Menu {
public:
	MenuGameMenu() : Menu(STR_TITLE_MAIN, 200, 98, 580 - 220, 143 - 113, false) {
		group.add(220 - 200, 113 - 98, STR_BTN_GAME_QUIT);
		group.add(220 - 200, 163 - 98, STR_BTN_GAME_ACHIEVEMENTS);
		group.add(220 - 200, 198 - 98, STR_BTN_GAME_INSTRUCTIONS);
		group.add(220 - 200, 233 - 98, STR_BTN_GAME_SAVE);
		group.add(220 - 200, 268 - 98, STR_BTN_GAME_LOAD);
		group.add(220 - 200, 303 - 98, STR_BTN_GAME_RESTART);
		group.add(220 - 200, 338 - 98, STR_BTN_GAME_SETTINGS);
		group.add(220 - 200, 373 - 98, STR_BTN_HELP);
		group.add(220 - 200, 408 - 98, STR_BTN_GAME_ABOUT);
		group.add(220 - 200, 458 - 98, STR_BTN_GAME_CANCEL);

		const Player *you = game.controlling_player();

		objects.emplace_back(
			new Background(
				DRS_BACKGROUND_GAME_0 + menu_bar_tbl[you->civ],
				200, 98, 600 - 200, 503 - 98
			)
		);
		objects.emplace_back(new Border(200, 98, 600 - 200, 503 - 98, false));
	}

	void button_group_activate(unsigned id) override final {
		switch (id) {
		case 0:
			in_game = 0;
			game.stop();
			ui_state.go_to(new MenuAchievements(1));
			break;
		case 1: ui_state.go_to(new MenuAchievements()); break;
		case 6: ui_state.go_to(new MenuGameSettings()); break;
		case 9: stop = 1; canvas.clear(); break;
		}
	}

	void draw() const override {
		canvas.dump_screen();
		Menu::draw();
	}
};

class MenuScenarioEditor final : public Menu {
	Palette palette;
	AnimationTexture menu_bar;
public:
	MenuScenarioEditor(const char *path)
		: Menu(STR_TITLE_MAIN, 0, 0, 110, 22, false)
		, palette(), menu_bar()
	{
		objects.emplace_back(new Border(0, 0, gfx_cfg.width, 51, false));
		objects.emplace_back(new Border(0, 457, gfx_cfg.width, gfx_cfg.height - 457, false));

		group.add(739, 5, STR_BTN_SCENARIO_MENU, 797 - 739, 45 - 5, true);
		group.add(765, 565, STR_BTN_HELP, 30, 30, true);

		ButtonRadioGroup *rgroup = new ButtonRadioGroup(false, 0, 0, 110, 22);
		rgroup->add(2, 2, STR_BTN_SCENARIO_MAP, true);
		rgroup->add(113, 2, STR_BTN_SCENARIO_TERRAIN, true);
		rgroup->add(224, 2, STR_BTN_SCENARIO_PLAYERS, true);
		rgroup->add(335, 2, STR_BTN_SCENARIO_UNITS, true);
		rgroup->add(446, 2, STR_BTN_SCENARIO_DIPLOMACY, true);
		rgroup->add(2, 26, STR_BTN_SCENARIO_TRIGGERS, true);
		rgroup->add(113, 26, STR_BTN_SCENARIO_TRIGGERS_ALL, true);
		rgroup->add(224, 26, STR_BTN_SCENARIO_OPTIONS, true);
		rgroup->add(335, 26, STR_BTN_SCENARIO_MESSAGES, true);
		rgroup->add(446, 26, STR_BTN_SCENARIO_VIDEO, true);
		objects.emplace_back(rgroup);

		palette.open(DRS_MAIN_PALETTE);
		// load background preferences
		Background b(DRS_BACKGROUND_SCENARIO_EDITOR, 0, 0);
		menu_bar.open(&palette, DRS_MENU_BAR_SCENARIO_800_600);

		editor.reshape(0, 50, gfx_cfg.width, gfx_cfg.height - (gfx_cfg.height - 457) - 50);
		editor.start(path);
	}

	void button_group_activate(unsigned id) override final {
		switch (id) {
		case 0:
			ui_state.go_to(new MenuScenarioEditorMenu());
			break;
		}
	}

	void keydown(SDL_KeyboardEvent *event) override {
		if (!game.keydown(event))
			Menu::keydown(event);
	}

	void keyup(SDL_KeyboardEvent *event) override {
		if (!game.keyup(event))
			Menu::keyup(event);
	}

	bool mousedown(SDL_MouseButtonEvent *event) override final {
		/* Check if mouse is within viewport. */
		int top = 50;
		int bottom = gfx_cfg.height - 457 - 50;

		if (game.paused || event->y < top || event->y >= bottom)
			return Menu::mousedown(event);

		//dbgf("top,bottom: %d,%d\n", top, bottom);
		event->y -= top;
		return game.mousedown(event);
	}

	void idle() override final {
		editor.idle();
	}

	void draw() const override final {
		canvas.clear();

		editor.draw();

		menu_bar.draw(0, 0, gfx_cfg.width, 50, 0);
		menu_bar.draw(0, 457, gfx_cfg.width, gfx_cfg.height - 457, 0);

		Menu::draw();
	}
};

class MenuGame final : public Menu {
	Palette palette;
	AnimationTexture menu_bar;
	Text str_paused;
public:
	MenuGame()
		: Menu(STR_TITLE_MAIN, 0, 0, 728 - 620, 18, false)
		, palette(), menu_bar()
		, str_paused(0, 0, STR_PAUSED, MIDDLE, CENTER, fnt_large)
	{
		group.add(728, 0, STR_BTN_MENU, gfx_cfg.width - 728, 18, true);
		group.add(620, 0, STR_BTN_DIPLOMACY, 728 - 620, 18, true);

		objects.emplace_back(new Text(gfx_cfg.width / 2, 3, STR_AGE_STONE, MIDDLE, TOP));

		const Player *you = game.controlling_player();

		char buf[32];
		int x = 32;
		snprintf(buf, sizeof buf, "%u", you->resources.food);
		objects.emplace_back(new Text(x, 3, buf));
		x = 98;
		snprintf(buf, sizeof buf, "%u", you->resources.wood);
		objects.emplace_back(new Text(x, 3, buf));
		x = 166;
		snprintf(buf, sizeof buf, "%u", you->resources.gold);
		objects.emplace_back(new Text(x, 3, buf));
		x = 234;
		snprintf(buf, sizeof buf, "%u", you->resources.stone);
		objects.emplace_back(new Text(x, 3, buf));

		// Lower HUD stuff
		objects.emplace_back(new Button(765, 482, 795 - 765, 512 - 482, STR_BTN_SCORE, true));
		objects.emplace_back(new Button(765, 564, 795 - 765, 594 - 564, "?", true));

		for (unsigned j = 0, y = 482; j < 2; ++j, y += 58)
			for (unsigned i = 0, x = 136; i < 6; ++i, x += 54) {
				Button *b = new Button(x, y, 54, 535 - 482, " ", true);
				b->sfx = SFX_BUTTON_GAME;
				objects.emplace_back(b);
			}

		canvas.clear();

		palette.open(DRS_MAIN_PALETTE);
		// load background preferences
		Background b(DRS_BACKGROUND_GAME_0 + menu_bar_tbl[you->civ], 0, 0);
		menu_bar.open(
			&palette,
			DRS_MENU_BAR_800_600_0 + menu_bar_tbl[you->civ]
		);

		int top = menu_bar.images[0].surface->h;
		int top2 = menu_bar.images[1].surface->h;

		str_paused.move(
			gfx_cfg.width / 2,
			top + (gfx_cfg.height - 126 - top) / 2,
			CENTER, MIDDLE
		);

		game.reshape(0, top, gfx_cfg.width, gfx_cfg.height - top - top2);
		game.start();
	}

	void button_group_activate(unsigned id) override final {
		canvas.read_screen();
		switch (id) {
		case 0:
			ui_state.go_to(new MenuGameMenu());
			break;
		}
	}

	void button_activate(unsigned id) override final {
		game.button_activate(id);
	}

	void keydown(SDL_KeyboardEvent *event) override {
		if (!game.keydown(event))
			Menu::keydown(event);
	}

	void keyup(SDL_KeyboardEvent *event) override {
		if (!game.keyup(event))
			Menu::keyup(event);
	}

	bool mousedown(SDL_MouseButtonEvent *event) override final {
		/* Check if mouse is within viewport. */
		int top = menu_bar.images[0].surface->h;
		int bottom = gfx_cfg.height - menu_bar.images[1].surface->h;

		if (game.paused || event->y < top || event->y >= bottom)
			return Menu::mousedown(event);

		//dbgf("top,bottom: %d,%d\n", top, bottom);
		event->y -= top;
		return game.mousedown(event);
	}

	void idle() override final {
		game.idle();
	}

	void draw() const override final {
		canvas.clear();

		game.draw();

		/* Draw HUD */
		int bottom = gfx_cfg.height - menu_bar.images[1].surface->h;

		canvas.col(0);

		SDL_Rect pos = {0, 0 + bottom, gfx_cfg.width, gfx_cfg.height};
		SDL_RenderFillRect(renderer, &pos);

		// draw background layers
		menu_bar.draw(0, 0, 0);
		menu_bar.draw(0, bottom, 1);
		// enable/disable hud buttons
		unsigned state = game.hud_mask();

		for (unsigned mask = 1, i = 0, j = 7; i < 6 * 2; ++i, mask <<= 1, ++j) {
			Button *b = reinterpret_cast<Button*>(objects[j].get());
			b->visible = state & mask;
		}

		// draw buttons
		Menu::draw();

		game.draw_hud(gfx_cfg.width, gfx_cfg.height);

		if (game.paused)
			str_paused.draw();
	}
};

class MenuSinglePlayerSettingsScenario final : public Menu {
public:
	MenuSinglePlayerSettingsScenario() : Menu(STR_TITLE_SINGLEPLAYER_SCENARIO, 0, 0, 386 - 87, 586 - 550, false) {
		objects.emplace_back(bkg = new Background(DRS_BACKGROUND_SINGLEPLAYER, 0, 0));
		objects.emplace_back(new Border(0, 0, gfx_cfg.width, gfx_cfg.height, false));
		objects.emplace_back(new Text(
			gfx_cfg.width / 2, 12, STR_TITLE_SINGLEPLAYER_SCENARIO, MIDDLE, TOP, fnt_button
		));

		objects.emplace_back(new Border(25, 75, 750, 104 - 75, true, true));
		objects.emplace_back(new Text(29, 53, STR_BTN_SCENARIO, LEFT, TOP, fnt_button));
		objects.emplace_back(new Text(702, 53, STR_PLAYERS, LEFT, TOP, fnt_button));

		group.add(87, 550, STR_BTN_OK);
		group.add(412, 550, STR_BTN_CANCEL);
	}

	void button_group_activate(unsigned) override final {
		stop = 1;
	}
};

class MenuSinglePlayerSettings final : public Menu {
	SelectorArea *sel_players;
	Text *txt_computer;
public:
	MenuSinglePlayerSettings() : Menu(STR_TITLE_SINGLEPLAYER, 0, 0, 386 - 87, 586 - 550, false) {
		objects.emplace_back(bkg = new Background(DRS_BACKGROUND_SINGLEPLAYER, 0, 0));
		objects.emplace_back(new Border(0, 0, gfx_cfg.width, gfx_cfg.height, false));
		objects.emplace_back(new Text(
			gfx_cfg.width / 2, 12, STR_TITLE_SINGLEPLAYER, MIDDLE, TOP, fnt_button
		));
		objects.emplace_back(new Button(779, 4, 795 - 779, 16, STR_EXIT, true));

		group.add(87, 550, STR_BTN_START_GAME);
		group.add(412, 550, STR_BTN_CANCEL);
		group.add(525, 62, STR_BTN_SETTINGS, 786 - 525, 98 - 62);

		// TODO add missing UI objects

		// setup players
		game.reset();

		objects.emplace_back(new Text(37, 72, STR_PLAYER_NAME, LEFT, TOP, fnt_button));
		objects.emplace_back(new Text(240, 72, STR_PLAYER_CIVILIZATION, LEFT, TOP, fnt_button));
		objects.emplace_back(new Text(363, 72, STR_PLAYER_ID, LEFT, TOP, fnt_button));
		objects.emplace_back(new Text(467, 72, STR_PLAYER_TEAM, LEFT, TOP, fnt_button));
		objects.emplace_back(new Text(39, 110, STR_PLAYER_YOU));
		objects.emplace_back(txt_computer = new Text(39, -140, STR_PLAYER_COMPUTER));

		objects.emplace_back(sel_players = new SelectorArea(38, 402, 125 - 38, 30, STR_PLAYER_COUNT, -2, -30));
		char buf[2] = "0";
		for (unsigned i = 1; i <= MAX_PLAYER_COUNT; ++i) {
			buf[0] = '0' + i;
			sel_players->add(buf);
		}
		sel_players->select(1);

		objects.emplace_back(new Button(387, 106, 424 - 387, 25, "1", true, true, SFX_BUTTON4, false, CENTER, MIDDLE, col_players[0]));
		objects.emplace_back(new Button(475, 106, 424 - 387, 25, "-"));
	}

	void button_group_activate(unsigned id) override final {
		switch (id) {
		case 0:
			ui_state.go_to(new MenuGame());
			break;
		case 1:
			stop = 1;
			break;
		case 2:
			ui_state.go_to(new MenuSinglePlayerSettingsScenario());
			break;
		}
	}

	void button_activate(unsigned id) override final {
		switch (id) {
		case 3: running = 0; break;
		}
	}

	void draw() const override final {
		Menu::draw();
		for (unsigned i = 0, n = sel_players->focus(); i < n; ++i)
			txt_computer->draw(txt_computer->x, 140 + 30 * i);
	}
};

void walk_campaign_item(void *arg, char *name);

class MenuCampaignSelectScenario final : public Menu {
	SelectorArea *sel_cpn, *sel_scn, *sel_lvl;
	std::vector<std::unique_ptr<Campaign>> campaigns;
public:
	MenuCampaignSelectScenario() : Menu(STR_TITLE_CAMPAIGN_SELECT_SCENARIO, 88, 550, 300, 36, false) {
		char buf[FS_BUFSZ];

		objects.emplace_back(bkg = new Background(DRS_BACKGROUND_SINGLEPLAYER, 0, 0));
		objects.emplace_back(new Border(0, 0, gfx_cfg.width, gfx_cfg.height, false));

		objects.emplace_back(new Button(779, 4, 795 - 779, 16, STR_EXIT, true));

		objects.emplace_back(new Text(
			gfx_cfg.width / 2, 8, STR_BTN_CAMPAIGN, MIDDLE, TOP, fnt_large
		));
		objects.emplace_back(sel_cpn = new SelectorArea(25, 87, 750, 249 - 87, STR_SELECT_CAMPAIGN, -4, -24));
		objects.emplace_back(sel_scn = new SelectorArea(25, 287, 750, 449 - 287, STR_SELECT_SCENARIO, -4, -24));
		objects.emplace_back(sel_lvl = new SelectorArea(25, 487, 187 - 25, 30, STR_SCENARIO_LEVEL, -4, -24));
		sel_lvl->add(STR_SCENARIO_LEVEL_EASIEST);
		sel_lvl->add(STR_SCENARIO_LEVEL_EASY);
		sel_lvl->add(STR_SCENARIO_LEVEL_MODERATE);
		sel_lvl->add(STR_SCENARIO_LEVEL_HARD);
		sel_lvl->add(STR_SCENARIO_LEVEL_HARDEST);
		sel_lvl->select(2);

		std::vector<std::string> files;
		fs_walk_campaign(walk_campaign_item, &files, buf, sizeof buf);

		group.add(0, 0, STR_BTN_OK);
		group.add(413 - 88, 0, STR_BTN_CANCEL);

		if (!files.size())
			// TODO show error
			stop = 1;
		else {
			std::sort(files.begin(), files.end(), [](const std::string &a, const std::string &b) {
				return strcasecmp(a.c_str(), b.c_str()) < 0;
			});
			for (auto &x : files)
				add_campaign(x.c_str());
			select_campaign(0);
		}
	}

	void add_campaign(const char *name) {
		dbgf("add campaign: %s\n", name);
		try {
			Campaign *c = new Campaign(name);
			campaigns.emplace_back(c);
			sel_cpn->add(c->name);
		} catch (int i) {
			panicf("bad campaign. code: %d\n", i);
		}
	}

	void button_group_activate(unsigned id) override final {
		dbgf("%s: id=%u\n", __func__, id);
		switch (id) {
		case 0:
		case 1:
			dbgf("todo: start campaign \"%s\", scenario \"%s\", level %s\n", campaigns[sel_cpn->focus()]->name, sel_scn->focus_text().c_str(), sel_lvl->focus_text().c_str());
			stop = 1;
			break;
		}
	}

	void button_activate(unsigned id) override final {
		switch (id) {
		case 2: running = 0; break;
		}
	}

	bool mousedown(SDL_MouseButtonEvent *event) override final {
		unsigned campaign_index = sel_cpn->focus();
		bool b = Menu::mousedown(event);
		if (!b)
			return false;
		unsigned index = sel_cpn->focus();
		if (index != campaign_index)
			select_campaign(index);

		return b;
	}

private:
	void select_campaign(unsigned index) {
		sel_scn->clear();
		size_t count;
		const struct cpn_scn *scn = campaigns[index]->scenarios(count);
		for (; count; --count, ++scn) {
			char buf[CPN_SCN_DESCRIPTION_MAX + 1];
			memcpy(buf, scn->description, CPN_SCN_DESCRIPTION_MAX);
			buf[CPN_SCN_DESCRIPTION_MAX] = '\0';
			sel_scn->add(buf);
		}
	}
};

void walk_campaign_item(void *arg, char *name)
{
	((std::vector<std::string>*)arg)->emplace_back(name);
}

class MenuCampaignPlayerSelection final : public Menu {
public:
	MenuCampaignPlayerSelection() : Menu(STR_TITLE_CAMPAIGN_SELECT_PLAYER, 0, 0, 300, 586 - 550, false) {
		objects.emplace_back(bkg = new Background(DRS_BACKGROUND_SINGLEPLAYER, 0, 0));
		objects.emplace_back(new Border(0, 0, gfx_cfg.width, gfx_cfg.height, false));
		objects.emplace_back(new Border(75, 125, 487 - 75, 500 - 125));

		objects.emplace_back(new Button(779, 4, 795 - 779, 16, STR_EXIT, true));

		objects.emplace_back(new Text(
			gfx_cfg.width / 2, 8, STR_TITLE_CAMPAIGN_SELECT_PLAYER, MIDDLE, TOP, fnt_large
		));

		objects.emplace_back(new Text(80, 100, STR_CAMPAIGN_PLAYER_NAME, LEFT, TOP, fnt_button));

		group.add(88, 550, STR_BTN_OK);
		group.add(413, 550, STR_BTN_CANCEL);
		group.add(500, 125, STR_BTN_CAMPAIGN_NEW_PLAYER, 725 - 500, 163 - 125);
		group.add(500, 175, STR_BTN_CAMPAIGN_REMOVE_PLAYER, 725 - 500, 163 - 125);
	}

	void button_group_activate(unsigned id) override final {
		switch (id) {
		case 0:
			ui_state.go_to(new MenuCampaignSelectScenario());
			break;
		case 1: stop = 1; break;
		}
	}

	void button_activate(unsigned id) override final {
		switch (id) {
		case 3: running = 0; break;
		}
	}
};

// FIXME color scheme
class MenuSinglePlayer final : public Menu {
public:
	MenuSinglePlayer() : Menu(STR_TITLE_SINGLEPLAYER_MENU, false) {
		objects.emplace_back(bkg = new Background(DRS_BACKGROUND_SINGLEPLAYER, 0, 0));
		objects.emplace_back(new Border(0, 0, gfx_cfg.width, gfx_cfg.height, false));
		objects.emplace_back(new Text(
			gfx_cfg.width / 2, 12, STR_TITLE_SINGLEPLAYER_MENU, MIDDLE, TOP, fnt_button
		));
		objects.emplace_back(new Button(779, 4, 795 - 779, 16, STR_EXIT, true));

		group.add(0, 147 - 222, STR_BTN_RANDOM_MAP);
		group.add(0, 210 - 222, STR_BTN_CAMPAIGN);
		group.add(0, 272 - 222, STR_BTN_DEATHMATCH);
		group.add(0, 335 - 222, STR_BTN_SCENARIO);
		group.add(0, 397 - 222, STR_BTN_SAVEDGAME);
		group.add(0, 460 - 222, STR_BTN_CANCEL);
	}

	void button_group_activate(unsigned id) override final {
		switch (id) {
		case 0:
			ui_state.go_to(new MenuSinglePlayerSettings());
			break;
		case 1:
			ui_state.go_to(new MenuCampaignPlayerSelection());
			break;
		case 5:
			stop = 1;
			break;
		}
	}

	void button_activate(unsigned) override final {
		running = 0;
	}
};

class MenuMultiplayer final : public Menu {
public:
	MenuMultiplayer() : Menu(STR_TITLE_MULTIPLAYER, 87, 550, 387 - 87, 587 - 550) {
		objects.emplace_back(bkg = new Background(DRS_BACKGROUND_MULTIPLAYER, 0, 0));
		objects.emplace_back(new Border(0, 0, gfx_cfg.width, gfx_cfg.height, false));
		objects.emplace_back(new Text(
			gfx_cfg.width / 2, 12, STR_TITLE_MULTIPLAYER, MIDDLE, TOP, fnt_button
		));
		objects.emplace_back(new Button(779, 4, 795 - 779, 16, STR_EXIT, true));

		objects.emplace_back(new Text(30, 96, STR_MULTIPLAYER_NAME, LEFT, TOP, fnt_button));
		objects.emplace_back(new Text(30, 209, STR_MULTIPLAYER_TYPE, LEFT, TOP, fnt_button));

		group.add(0, 0, STR_BTN_OK);
		group.add(412 - 87, 0, STR_BTN_CANCEL);
	}

	void button_group_activate(unsigned id) override final {
		switch (id) {
		case 1:
			stop = 1;
			break;
		}
	}

	void button_activate(unsigned) override final {
		running = 0;
	}
};

class MenuScenarioBuilder final : public Menu {
public:
	MenuScenarioBuilder() : Menu(STR_TITLE_SCENARIO_EDITOR) {
		objects.emplace_back(bkg = new Background(DRS_BACKGROUND_SCENARIO, 0, 0));
		objects.emplace_back(new Border(0, 0, gfx_cfg.width, gfx_cfg.height, false));
		objects.emplace_back(new Text(
			gfx_cfg.width / 2, 12, STR_TITLE_SCENARIO_EDITOR, MIDDLE, TOP, fnt_button
		));
		objects.emplace_back(new Button(779, 4, 795 - 779, 16, STR_EXIT, true));

		group.add(0, 212 - 222, STR_BTN_SCENARIO_CREATE);
		group.add(0, 275 - 222, STR_BTN_SCENARIO_EDIT);
		group.add(0, 337 - 222, STR_BTN_SCENARIO_CAMPAIGN);
		group.add(0, 400 - 222, STR_BTN_SCENARIO_CANCEL);
	}

	void button_group_activate(unsigned id) override final {
		switch (id) {
		case 0:
			ui_state.go_to(new MenuScenarioEditor(NULL));
			break;
		case 1:
			ui_state.go_to(new MenuFileSelection(STR_SELECT_SCENARIO, DRS_BACKGROUND_SCENARIO, "scenario/", "scn", FS_OPT_NEED_GAME));
			break;
		case 3:
			stop = 1;
			break;
		}
	}

	void button_activate(unsigned) override final {
		running = 0;
	}

	void restore() override final {
		Menu::restore();
		if (ui_state.file_selected.size()) {
			ui_state.go_to(new MenuScenarioEditor(ui_state.file_selected.c_str()));
			ui_state.file_selected.clear();
		}
	}
};

class MenuMain final : public Menu {
public:
	MenuMain() : Menu(STR_TITLE_MAIN, false) {
		objects.emplace_back(bkg = new Background(DRS_BACKGROUND_MAIN, 0, 0));
		objects.emplace_back(new Border(0, 0, gfx_cfg.width, gfx_cfg.height, false));

		group.add(0, 0, STR_BTN_SINGLEPLAYER);
		group.add(0, 285 - 222, STR_BTN_MULTIPLAYER);
		group.add(0, 347 - 222, STR_BTN_HELP);
		group.add(0, 410 - 222, STR_BTN_EDIT);
		group.add(0, 472 - 222, STR_BTN_EXIT);

		// FIXME (tm) probably gets truncated by resource handling in res.h (ascii, unicode stuff)
		// XXX or it may even use a separate slp or gfx object to draw this...
		objects.emplace_back(new Text(gfx_cfg.width / 2, 542, STR_MAIN_COPY1, CENTER));
		// FIXME (copy) and (p) before this line
		objects.emplace_back(new Text(gfx_cfg.width / 2, 561, STR_MAIN_COPY2, CENTER));
		objects.emplace_back(new Text(gfx_cfg.width / 2, 578, STR_MAIN_COPY3, CENTER));

		mus_play(MUS_MAIN);
	}

	void button_group_activate(unsigned id) override final {
		switch (id) {
		case 0:
			ui_state.go_to(new MenuSinglePlayer());
			break;
		case 1:
			ui_state.go_to(new MenuMultiplayer());
			break;
		case 2:
			open_readme();
			break;
		case 3:
			ui_state.go_to(new MenuScenarioBuilder());
			break;
		case 4:
			running = 0;
			break;
		}
	}
};

extern "C"
{

void ui_init()
{
	canvas.renderer = renderer;
	ui_state.go_to(new MenuMain());
}

void ui_free(void)
{
	running = 0;
	ui_state.dispose();
	game.dispose();
}

/* Wrappers for ui_state */
void idle() { ui_state.idle(); }

void display() { ui_state.display(); }

void repaint()
{
	ui_state.dirty();
	display();
}

void keydown(SDL_KeyboardEvent *event) { ui_state.keydown(event); }
void keyup(SDL_KeyboardEvent *event) { ui_state.keyup(event); }

void mousedown(SDL_MouseButtonEvent *event) { ui_state.mousedown(event); }
void mouseup(SDL_MouseButtonEvent *event) { ui_state.mouseup(event); }

}

void UI_State::mousedown(SDL_MouseButtonEvent *event)
{
	if (navigation.empty())
		return;

	navigation.top().get()->mousedown(event);
}

void UI_State::mouseup(SDL_MouseButtonEvent *event)
{
	if (navigation.empty())
		return;

	auto menu = navigation.top().get();
	menu->mouseup(event);

	update();
}

void UI_State::go_to(Menu *menu)
{
	navigation.emplace(menu);
}

void UI_State::dispose()
{
	while (!navigation.empty())
		navigation.pop();
}

void UI_State::idle()
{
	if (navigation.empty())
		return;

	navigation.top()->idle();
}

void UI_State::display()
{
	if ((state & DIRTY) && !navigation.empty()) {
		// Draw stuff
		navigation.top()->draw();
		// Swap buffers
		SDL_RenderPresent(renderer);
	}

	if (!running)
		dispose();

	state &= ~DIRTY;
}

void UI_State::keydown(SDL_KeyboardEvent *event)
{
	if (navigation.empty())
		return;

	navigation.top().get()->keydown(event);
}

void UI_State::keyup(SDL_KeyboardEvent *event)
{
	if (navigation.empty())
		return;

	if (event->keysym.sym == SDLK_F10) {
		canvas.save_screen();
		return;
	}

	auto menu = navigation.top().get();
	menu->keyup(event);

	update();
}

void UI_State::update()
{
	for (unsigned n = navigation.top()->stop; n; --n) {
		if (navigation.empty())
			return;
		navigation.pop();
	}
	navigation.top()->restore();
}
