/* Copyright 2016-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "ui.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <cstdio>
#include <cstddef>
#include <cstdlib>
#include <cstring>

#include <memory>
#include <string>
#include <stack>
#include <vector>

#include "../setup/dbg.h"
#include "../setup/def.h"
#include "../setup/res.h"

#include "drs.h"
#include "gfx.h"
#include "lang.h"
#include "sfx.h"
#include "fs.h"

#include "image.hpp"
#include "game.hpp"

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
	UI_State() : state(DIRTY), navigation() {}

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
	if (!(pixels = malloc(WIDTH * HEIGHT * 3)))
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
	SDL_Surface *screen;
	int err = 1;

	// FIXME support big endian byte order
	if (!(screen = SDL_CreateRGBSurface(0, WIDTH, HEIGHT, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000)))
		goto fail;
	if (SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ARGB8888, screen->pixels, screen->pitch))
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
	if (!(capture = SDL_CreateRGBSurface(0, WIDTH, HEIGHT, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000)))
		panic("read_screen");

	SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ARGB8888, capture->pixels, capture->pitch);

	if (!(tex = SDL_CreateTextureFromSurface(renderer, capture)))
		panic("read_screen");
}

void Renderer::dump_screen() {
	SDL_Rect pos = {0, 0, WIDTH, HEIGHT};
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
protected:
	int x, y;
	unsigned w, h;

public:
	UI(int x, int y, unsigned w=1, unsigned h=1)
		: x(x), y(y), w(w), h(h) {}
	virtual ~UI() {}

	virtual void draw() const = 0;

	bool contains(int px, int py) {
		return point_in_rect(x, y, w, h, px, py);
	}
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
	std::string str;
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
	bool fill;
	int shade;
public:
	Border(int x, int y, unsigned w=1, unsigned h=1, bool fill=true)
		: UI(x, y, w, h), fill(fill), shade(-1) {}

	Border(int x, int y, unsigned w, unsigned h, int shade)
		: UI(x, y, w, h), fill(true), shade(shade) {}

	void draw() const {
		draw(false);
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

unsigned cmd_or_next(const unsigned char **cmd, unsigned n)
{
	const unsigned char *ptr = *cmd;
	unsigned v = *ptr >> n;
	if (!v)
		v = *(++ptr);
	*cmd = ptr;
	return v;
}

Image::Image() : surface(NULL), texture(NULL) {}

bool Image::load(Palette *pal, const void *data, const struct slp_frame_info *frame, unsigned player)
{
	bool dynamic = false;

	hotspot_x = frame->hotspot_x;
	hotspot_y = frame->hotspot_y;

	dbgf("player: %u\n", player);

	dbgf("dimensions: %u x %u\n", frame->width, frame->height);
	dbgf("hotspot: %u,%u\n", frame->hotspot_x, frame->hotspot_y);
	dbgf("command table offset: %X\n", frame->cmd_table_offset);
	dbgf("outline table offset: %X\n", frame->outline_table_offset);

	// FIXME big endian support
	surface = std::shared_ptr<SDL_Surface >(
		SDL_CreateRGBSurface(0, frame->width, frame->height, 8, 0, 0, 0, 0),
		SDL_FreeSurface
	);

	if (!surface || SDL_SetSurfacePalette(surface.get(), pal->pal))
		panicf("Cannot create image: %s\n", SDL_GetError());
	if (surface->format->format != SDL_PIXELFORMAT_INDEX8)
		panicf("Unexpected image format: %s\n", SDL_GetPixelFormatName(surface->format->format));

	// fill with random data so we can easily spot garbled image data
	unsigned char *pixels = (unsigned char*)surface->pixels;
	for (int y = 0, h = frame->height, p = surface->pitch; y < h; ++y)
		for (int x = 0, w = frame->width; x < w; ++x)
			pixels[y * p + x] = 0;

	const struct slp_frame_row_edge *edge =
		(const struct slp_frame_row_edge*)
			((char*)data + frame->outline_table_offset);
	const unsigned char *cmd = (const unsigned char*)
		((char*)data + frame->cmd_table_offset + 4 * frame->height);

	dbgf("command data offset: %X\n", frame->cmd_table_offset + 4 * frame->height);

	for (int y = 0, h = frame->height; y < h; ++y, ++edge) {
		if (edge->left_space == 0x8000 || edge->right_space == 0x8000)
			continue;

		int line_size = frame->width - edge->left_space - edge->right_space;
		#if 0
		printf("%8X: %3d: %d (%hu, %hu)\n",
			(unsigned)(cmd - (const unsigned char*)data),
			y, line_size, edge->left_space, edge->right_space
		);
		#endif

		// fill line_size with default value
		for (int x = edge->left_space, w = x + line_size, p = surface->pitch; x < w; ++x)
			pixels[y * p + x] = rand();

		for (int i = edge->left_space, x = i, w = x + line_size, p = surface->pitch; i <= w; ++i, ++cmd) {
			unsigned command, count;

			command = *cmd & 0x0f;

			// TODO figure out if lesser skip behaves different compared to aoe2

			switch (*cmd) {
			case 0x03:
				for (count = *++cmd; count; --count)
					pixels[y * p + x++] = 0;
				continue;
			// TODO what does this do?
			// probably fills as much as the next byte says
			case 0x02:
				for (count = *++cmd; count; --count)
					pixels[y * p + x++] = *++cmd;
				continue;
			case 0xF7: pixels[y * p + x++] = cmd[1];
			case 0xE7: pixels[y * p + x++] = cmd[1];
			case 0xD7: pixels[y * p + x++] = cmd[1];
			case 0xC7: pixels[y * p + x++] = cmd[1];
			case 0xB7: pixels[y * p + x++] = cmd[1];
			// dup 10
			case 0xA7: pixels[y * p + x++] = cmd[1];
			case 0x97: pixels[y * p + x++] = cmd[1];
			case 0x87: pixels[y * p + x++] = cmd[1];
			case 0x77: pixels[y * p + x++] = cmd[1];
			case 0x67: pixels[y * p + x++] = cmd[1];
			case 0x57: pixels[y * p + x++] = cmd[1];
			case 0x47: pixels[y * p + x++] = cmd[1];
			case 0x37: pixels[y * p + x++] = cmd[1];
			case 0x27: pixels[y * p + x++] = cmd[1];
			case 0x17: pixels[y * p + x++] = cmd[1];
				++cmd;
				continue;
			// player color fill
			case 0xF6: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 15
			case 0xE6: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 14
			case 0xD6: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 13
			case 0xC6: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 12
			case 0xB6: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 11
			case 0xA6: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 10
			case 0x96: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 9
			case 0x86: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 8
			case 0x76: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 7
			case 0x66: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 6
			case 0x56: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 5
			case 0x46: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 4
			case 0x36: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 3
			case 0x26: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 2
			case 0x16: pixels[y * p + x++] = *++cmd + 0x10 * (player + 1); // player fill 1
				dynamic = true;
				continue;
			// XXX pixel count if lower_nibble == 4: 1 + cmd >> 2
			case 0xfc: pixels[y * p + x++] = *++cmd; // fill 63
			case 0xf8: pixels[y * p + x++] = *++cmd; // fill 62
			case 0xf4: pixels[y * p + x++] = *++cmd; // fill 61
			case 0xf0: pixels[y * p + x++] = *++cmd; // fill 60
			case 0xec: pixels[y * p + x++] = *++cmd; // fill 59
			case 0xe8: pixels[y * p + x++] = *++cmd; // fill 58
			case 0xe4: pixels[y * p + x++] = *++cmd; // fill 57
			case 0xe0: pixels[y * p + x++] = *++cmd; // fill 56
			case 0xdc: pixels[y * p + x++] = *++cmd; // fill 55
			case 0xd8: pixels[y * p + x++] = *++cmd; // fill 54
			case 0xd4: pixels[y * p + x++] = *++cmd; // fill 53
			case 0xd0: pixels[y * p + x++] = *++cmd; // fill 52
			case 0xcc: pixels[y * p + x++] = *++cmd; // fill 51
			case 0xc8: pixels[y * p + x++] = *++cmd; // fill 50
			case 0xc4: pixels[y * p + x++] = *++cmd; // fill 49
			case 0xc0: pixels[y * p + x++] = *++cmd; // fill 48
			case 0xbc: pixels[y * p + x++] = *++cmd; // fill 47
			case 0xb8: pixels[y * p + x++] = *++cmd; // fill 46
			case 0xb4: pixels[y * p + x++] = *++cmd; // fill 45
			case 0xb0: pixels[y * p + x++] = *++cmd; // fill 44
			case 0xac: pixels[y * p + x++] = *++cmd; // fill 43
			case 0xa8: pixels[y * p + x++] = *++cmd; // fill 42
			case 0xa4: pixels[y * p + x++] = *++cmd; // fill 41
			case 0xa0: pixels[y * p + x++] = *++cmd; // fill 40
			case 0x9c: pixels[y * p + x++] = *++cmd; // fill 39
			case 0x98: pixels[y * p + x++] = *++cmd; // fill 38
			case 0x94: pixels[y * p + x++] = *++cmd; // fill 37
			case 0x90: pixels[y * p + x++] = *++cmd; // fill 36
			case 0x8c: pixels[y * p + x++] = *++cmd; // fill 35
			case 0x88: pixels[y * p + x++] = *++cmd; // fill 34
			case 0x84: pixels[y * p + x++] = *++cmd; // fill 33
			case 0x80: pixels[y * p + x++] = *++cmd; // fill 32
			case 0x7c: pixels[y * p + x++] = *++cmd; // fill 31
			case 0x78: pixels[y * p + x++] = *++cmd; // fill 30
			case 0x74: pixels[y * p + x++] = *++cmd; // fill 29
			case 0x70: pixels[y * p + x++] = *++cmd; // fill 28
			case 0x6c: pixels[y * p + x++] = *++cmd; // fill 27
			case 0x68: pixels[y * p + x++] = *++cmd; // fill 26
			case 0x64: pixels[y * p + x++] = *++cmd; // fill 25
			case 0x60: pixels[y * p + x++] = *++cmd; // fill 24
			case 0x5c: pixels[y * p + x++] = *++cmd; // fill 23
			case 0x58: pixels[y * p + x++] = *++cmd; // fill 22
			case 0x54: pixels[y * p + x++] = *++cmd; // fill 21
			case 0x50: pixels[y * p + x++] = *++cmd; // fill 20
			case 0x4c: pixels[y * p + x++] = *++cmd; // fill 19
			case 0x48: pixels[y * p + x++] = *++cmd; // fill 18
			case 0x44: pixels[y * p + x++] = *++cmd; // fill 17
			case 0x40: pixels[y * p + x++] = *++cmd; // fill 16
			case 0x3c: pixels[y * p + x++] = *++cmd; // fill 15
			case 0x38: pixels[y * p + x++] = *++cmd; // fill 14
			case 0x34: pixels[y * p + x++] = *++cmd; // fill 13
			case 0x30: pixels[y * p + x++] = *++cmd; // fill 12
			case 0x2c: pixels[y * p + x++] = *++cmd; // fill 11
			case 0x28: pixels[y * p + x++] = *++cmd; // fill 10
			case 0x24: pixels[y * p + x++] = *++cmd; // fill 9
			case 0x20: pixels[y * p + x++] = *++cmd; // fill 8
			case 0x1c: pixels[y * p + x++] = *++cmd; // fill 7
			case 0x18: pixels[y * p + x++] = *++cmd; // fill 6
			case 0x14: pixels[y * p + x++] = *++cmd; // fill 5
			case 0x10: pixels[y * p + x++] = *++cmd; // fill 4
			case 0x0c: pixels[y * p + x++] = *++cmd; // fill 3
			case 0x08: pixels[y * p + x++] = *++cmd; // fill 2
			case 0x04: pixels[y * p + x++] = *++cmd; // fill 1
				continue;
			case 0xfd: pixels[y * p + x++] = 0; // skip 63
			case 0xf9: pixels[y * p + x++] = 0; // skip 62
			case 0xf5: pixels[y * p + x++] = 0; // skip 61
			case 0xf1: pixels[y * p + x++] = 0; // skip 60
			case 0xed: pixels[y * p + x++] = 0; // skip 59
			case 0xe9: pixels[y * p + x++] = 0; // skip 58
			case 0xe5: pixels[y * p + x++] = 0; // skip 57
			case 0xe1: pixels[y * p + x++] = 0; // skip 56
			case 0xdd: pixels[y * p + x++] = 0; // skip 55
			case 0xd9: pixels[y * p + x++] = 0; // skip 54
			case 0xd5: pixels[y * p + x++] = 0; // skip 53
			case 0xd1: pixels[y * p + x++] = 0; // skip 52
			case 0xcd: pixels[y * p + x++] = 0; // skip 51
			case 0xc9: pixels[y * p + x++] = 0; // skip 50
			case 0xc5: pixels[y * p + x++] = 0; // skip 49
			case 0xc1: pixels[y * p + x++] = 0; // skip 48
			case 0xbd: pixels[y * p + x++] = 0; // skip 47
			case 0xb9: pixels[y * p + x++] = 0; // skip 46
			case 0xb5: pixels[y * p + x++] = 0; // skip 45
			case 0xb1: pixels[y * p + x++] = 0; // skip 44
			case 0xad: pixels[y * p + x++] = 0; // skip 43
			case 0xa9: pixels[y * p + x++] = 0; // skip 42
			case 0xa5: pixels[y * p + x++] = 0; // skip 41
			case 0xa1: pixels[y * p + x++] = 0; // skip 40
			case 0x9d: pixels[y * p + x++] = 0; // skip 39
			case 0x99: pixels[y * p + x++] = 0; // skip 38
			case 0x95: pixels[y * p + x++] = 0; // skip 37
			case 0x91: pixels[y * p + x++] = 0; // skip 36
			case 0x8d: pixels[y * p + x++] = 0; // skip 35
			case 0x89: pixels[y * p + x++] = 0; // skip 34
			case 0x85: pixels[y * p + x++] = 0; // skip 33
			case 0x81: pixels[y * p + x++] = 0; // skip 32
			case 0x7d: pixels[y * p + x++] = 0; // skip 31
			case 0x79: pixels[y * p + x++] = 0; // skip 30
			case 0x75: pixels[y * p + x++] = 0; // skip 29
			case 0x71: pixels[y * p + x++] = 0; // skip 28
			case 0x6d: pixels[y * p + x++] = 0; // skip 27
			case 0x69: pixels[y * p + x++] = 0; // skip 26
			case 0x65: pixels[y * p + x++] = 0; // skip 25
			case 0x61: pixels[y * p + x++] = 0; // skip 24
			case 0x5d: pixels[y * p + x++] = 0; // skip 23
			case 0x59: pixels[y * p + x++] = 0; // skip 22
			case 0x55: pixels[y * p + x++] = 0; // skip 21
			case 0x51: pixels[y * p + x++] = 0; // skip 20
			case 0x4d: pixels[y * p + x++] = 0; // skip 19
			case 0x49: pixels[y * p + x++] = 0; // skip 18
			case 0x45: pixels[y * p + x++] = 0; // skip 17
			case 0x41: pixels[y * p + x++] = 0; // skip 16
			case 0x3d: pixels[y * p + x++] = 0; // skip 15
			case 0x39: pixels[y * p + x++] = 0; // skip 14
			case 0x35: pixels[y * p + x++] = 0; // skip 13
			case 0x31: pixels[y * p + x++] = 0; // skip 12
			case 0x2d: pixels[y * p + x++] = 0; // skip 11
			case 0x29: pixels[y * p + x++] = 0; // skip 10
			case 0x25: pixels[y * p + x++] = 0; // skip 9
			case 0x21: pixels[y * p + x++] = 0; // skip 8
			case 0x1d: pixels[y * p + x++] = 0; // skip 7
			case 0x19: pixels[y * p + x++] = 0; // skip 6
			case 0x15: pixels[y * p + x++] = 0; // skip 5
			case 0x11: pixels[y * p + x++] = 0; // skip 4
			case 0x0d: pixels[y * p + x++] = 0; // skip 3
			case 0x09: pixels[y * p + x++] = 0; // skip 2
			case 0x05: pixels[y * p + x++] = 0; // skip 1
				continue;
			}

			switch (command) {
			case 0x0f:
				//dbgs("break");
				i = w;
				break;
			case 0x0a:
				count = cmd_or_next(&cmd, 4);
				for (++cmd; count; --count) {
					pixels[y * p + x++] = *cmd + 0x10 * (player + 1);
				}
				dynamic = true;
				break;
			case 0x07:
				count = cmd_or_next(&cmd, 4);
				//dbgf("fill: %u pixels\n", count);
				for (++cmd; count; --count) {
					//dbgf(" %x", (unsigned)(*cmd) & 0xff);
					pixels[y * p + x++] = *cmd;
				}
				//dbgs("");
				break;
			default:
				dbgf("unknown cmd at %X: %X, %X\n",
					 (unsigned)(cmd - (const unsigned char*)data),
					*cmd, command
				);
				// dump some more bytes
				for (size_t i = 0; i < 16; ++i)
					dbgf(" %02hhX", *((const unsigned char*)cmd + i));
				dbgs("");
				#if 1
				while (*cmd != 0xf)
					++cmd;
				i = w;
				#endif
				break;
			}
		}

		#if 0
		while (cmd[-1] != 0xf)
			++cmd;
		#endif
	}
	dbgs("");

	if (SDL_SetColorKey(surface.get(), SDL_TRUE, 0))
		fprintf(stderr, "Could not set transparency: %s\n", SDL_GetError());

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

	slp_read(&image, data);

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

		if (bkg_id[1] < 0)
			bkg_id[1] = bkg_id[0];

		animation.open(&palette, (unsigned)bkg_id[1]);
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

class Button final : public Border {
	Text text;
public:
	bool focus;
	bool down;
	bool play_sfx;
	bool visible;

	Button(int x, int y, unsigned w, unsigned h, unsigned id, bool def_fnt=false, bool play_sfx=true)
		: Border(x, y, w, h)
		, text(x + w / 2, y + h / 2, id, CENTER, MIDDLE, def_fnt ? fnt_default : fnt_button)
		, focus(false), down(false), play_sfx(play_sfx), visible(true)
	{
	}

	Button(int x, int y, unsigned w, unsigned h, const std::string &str, bool def_fnt=false, bool play_sfx=true)
		: Border(x, y, w, h)
		, text(x + w / 2, y + h / 2, str, CENTER, MIDDLE, def_fnt ? fnt_default : fnt_button)
		, focus(false), down(false), play_sfx(play_sfx), visible(true)
	{
	}

	/* Process mouse event if it has been clicked. */
	bool mousedown(SDL_MouseButtonEvent *event) {
		bool old_down = down;

		focus = down = visible && contains(event->x, event->y);

		if (old_down != down)
			ui_state.dirty();

		return down;
	}

	/* Process mouse event if it has been clicked. */
	bool mouseup(SDL_MouseButtonEvent *event) {
		if (down) {
			down = false;
			ui_state.dirty();

			bool hit = contains(event->x, event->y);
			if (hit && play_sfx)
				sfx_play(SFX_BUTTON4);
			return hit;
		}

		// Button wasn't pressed so we can't munge the event.
		return false;
	}

	void draw() const {
		if (!visible)
			return;
		Border::draw(down);
		text.draw(focus);
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

class Menu : public UI {
protected:
	std::vector<std::shared_ptr<UI>> objects;
	mutable ButtonGroup group;
	Background *bkg;
public:
	int stop = 0;

	Menu(unsigned title_id, bool show_title=true)
		: UI(0, 0, WIDTH, HEIGHT), objects(), group(), bkg(nullptr)
	{
		if (show_title)
			objects.emplace_back(new Text(
				WIDTH / 2, 12, title_id, MIDDLE, TOP, fnt_button
			));
	}

	Menu(unsigned title_id, int x, int y, unsigned w, unsigned h, bool show_title=true)
		: UI(0, 0, WIDTH, HEIGHT), objects(), group(x, y, w, h), bkg(nullptr)
	{
		if (show_title)
			objects.emplace_back(new Text(
				WIDTH / 2, 12, title_id, MIDDLE, TOP, fnt_button
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

	virtual void mousedown(SDL_MouseButtonEvent *event) {
		group.mousedown(event);

		for (auto x : objects) {
			Button *btn = dynamic_cast<Button*>(x.get());
			if (btn && btn->mousedown(event))
				group.release_focus();
		}
	}

	void mouseup(SDL_MouseButtonEvent *event) {
		if (group.mouseup(event)) {
			button_group_activate(group.focus);
			return;
		}

		unsigned id = 0;
		for (auto x : objects) {
			Button *btn = dynamic_cast<Button*>(x.get());
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

	MenuTimeline(unsigned type) : Menu(STR_TITLE_ACHIEVEMENTS, 0, 0, 550 - 250, 588 - 551) {
		objects.emplace_back(bkg = new Background(type ? type == 2 ? DRS_BACKGROUND_VICTORY : DRS_BACKGROUND_DEFEAT : DRS_BACKGROUND_ACHIEVEMENTS, 0, 0));
		objects.emplace_back(new Border(0, 0, WIDTH, HEIGHT, false));
		objects.emplace_back(new Button(779, 4, 795 - 779, 16, STR_EXIT, true));
		objects.emplace_back(new Text(
			WIDTH / 2, 12, STR_TITLE_ACHIEVEMENTS, MIDDLE, TOP, fnt_button
		));

		objects.emplace_back(new Text(WIDTH / 2, 48, STR_BTN_TIMELINE, CENTER, TOP));

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
public:
	MenuAchievements(unsigned type = 0) : Menu(STR_TITLE_ACHIEVEMENTS, 0, 0, type ? 775 - 550 : 375 - 125, 588 - 551), type(type) {
		objects.emplace_back(bkg = new Background(type ? type == 2 ? DRS_BACKGROUND_VICTORY : DRS_BACKGROUND_DEFEAT : DRS_BACKGROUND_ACHIEVEMENTS, 0, 0));
		objects.emplace_back(new Border(0, 0, WIDTH, HEIGHT, false));
		objects.emplace_back(new Button(779, 4, 795 - 779, 16, STR_EXIT, true));

		objects.emplace_back(new Text(WIDTH / 2, 48, STR_TITLE_SUMMARY, CENTER, TOP));

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

		objects.emplace_back(new Button(124, 187, 289 - 124, 212 - 187, STR_BTN_MILITARY, LEFT));
		objects.emplace_back(new Button(191, 162, 354 - 191, 187 - 162, STR_BTN_ECONOMY, LEFT));
		objects.emplace_back(new Button(266, 137, 420 - 266, 162 - 137, STR_BTN_RELIGION, LEFT));
		objects.emplace_back(new Button(351, 112, 549 - 351, 137 - 112, STR_BTN_TECHNOLOGY));
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
		case 3: running = 0; break;
		}
	}
};

class MenuGameSettings final : public Menu {
public:
	MenuGameSettings() : Menu(STR_TITLE_GAME_SETTINGS, 100, 105, 700 - 100, 495 - 105) {
		const Player *you = game.controlling_player();

		objects.emplace_back(
			new Background(
				DRS_BACKGROUND_GAME_0 + menu_bar_tbl[you->civ],
				100, 105, 700 - 100, 495 - 105
			)
		);

		objects.emplace_back(new Border(100, 105, 700 - 100, 495 - 105, false));

		group.add(220 - 100, 450 - 105, STR_BTN_OK, 390 - 220, 480 - 450);
		group.add(410 - 100, 450 - 105, STR_BTN_CANCEL, 580 - 410, 480 - 450);

		objects.emplace_back(new Text(125, 163, STR_TITLE_SPEED));
		objects.emplace_back(new Text(270, 154, STR_TITLE_MUSIC));
		objects.emplace_back(new Text(410, 154, STR_TITLE_SOUND));
		objects.emplace_back(new Text(550, 154, STR_TITLE_SCROLL));

		// TODO create and replace with checkbox
		objects.emplace_back(new Button(120, 190, 150 - 120, 220 - 190, STR_EXIT));
		objects.emplace_back(new Button(120, 225, 150 - 120, 255 - 225, STR_EXIT));
		objects.emplace_back(new Button(120, 260, 150 - 120, 290 - 260, STR_EXIT));

		objects.emplace_back(new Text(125, 303, STR_TITLE_SCREEN));
		objects.emplace_back(new Text(275, 303, STR_TITLE_MOUSE));
		objects.emplace_back(new Text(435, 303, STR_TITLE_HELP));
		objects.emplace_back(new Text(565, 303, STR_TITLE_PATH));
	}

	void button_group_activate(unsigned) override final {
		stop = 1;
	}

	void button_activate(unsigned) override final {
	}

	void draw() const override {
		canvas.dump_screen();
		Menu::draw();
	}
};

// TODO make overlayed menu
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

class MenuGame final : public Menu {
	Palette palette;
	AnimationTexture menu_bar;
	Text str_paused;
public:
	MenuGame()
		: Menu(STR_TITLE_MAIN, 0, 0, 728 - 620, 18, false)
		, palette(), menu_bar()
		, str_paused(0, 0, STR_PAUSED, MIDDLE, CENTER, fnt_button)
	{
		group.add(728, 0, STR_BTN_MENU, WIDTH - 728, 18, true);
		group.add(620, 0, STR_BTN_DIPLOMACY, 728 - 620, 18, true);

		objects.emplace_back(new Text(WIDTH / 2, 3, STR_AGE_STONE, MIDDLE, TOP));

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
				objects.emplace_back(b);
			}

		canvas.clear();

		palette.open(DRS_MAIN_PALETTE);
		menu_bar.open(
			&palette,
			DRS_MENU_BAR_800_600_0 + menu_bar_tbl[you->civ]
		);

		int top = menu_bar.images[0].surface->h;
		int top2 = menu_bar.images[1].surface->h;

		str_paused.move(
			WIDTH / 2,
			top + (HEIGHT - 126 - top) / 2,
			CENTER, MIDDLE
		);

		game.reshape(0, top, WIDTH, HEIGHT - top - top2);
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

	void mousedown(SDL_MouseButtonEvent *event) override final {
		/* Check if mouse is within viewport. */
		int top = menu_bar.images[0].surface->h;
		int bottom = HEIGHT - menu_bar.images[1].surface->h;

		if (game.paused || event->y < top || event->y >= bottom) {
			Menu::mousedown(event);
			return;
		}

		//dbgf("top,bottom: %d,%d\n", top, bottom);
		event->y -= top;
		game.mousedown(event);
	}

	void idle() override final {
		game.idle();
	}

	void draw() const override final {
		canvas.clear();

		game.draw();

		/* Draw HUD */
		int bottom = HEIGHT - menu_bar.images[1].surface->h;

		canvas.col(0);

		SDL_Rect pos = {0, bottom, WIDTH, HEIGHT};
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

		game.draw_hud(WIDTH, HEIGHT);

		if (game.paused)
			str_paused.draw();
	}
};

class MenuSinglePlayerSettings final : public Menu {
public:
	MenuSinglePlayerSettings() : Menu(STR_TITLE_SINGLEPLAYER, 0, 0, 386 - 87, 586 - 550, false) {
		objects.emplace_back(bkg = new Background(DRS_BACKGROUND_SINGLEPLAYER, 0, 0));
		objects.emplace_back(new Border(0, 0, WIDTH, HEIGHT, false));
		objects.emplace_back(new Text(
			WIDTH / 2, 12, STR_TITLE_SINGLEPLAYER, MIDDLE, TOP, fnt_button
		));
		objects.emplace_back(new Button(779, 4, 795 - 779, 16, STR_EXIT, true));

		group.add(87, 550, STR_BTN_START_GAME);
		group.add(412, 550, STR_BTN_CANCEL);
		group.add(525, 62, STR_BTN_SETTINGS, 786 - 525, 98 - 62);

		// TODO add missing UI objects

		// setup players
		game.reset();

		char str_count[2] = "0";
		str_count[0] += game.player_count();

		// FIXME hardcoded 2 player mode
		objects.emplace_back(new Text(38, 374, STR_PLAYER_COUNT, LEFT, TOP, fnt_button));
		objects.emplace_back(new Text(44, 410, str_count));
	}

	void button_group_activate(unsigned id) override final {
		switch (id) {
		case 0:
			ui_state.go_to(new MenuGame());
			break;
		case 1:
			stop = 1;
			break;
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
		objects.emplace_back(new Border(0, 0, WIDTH, HEIGHT, false));
		objects.emplace_back(new Text(
			WIDTH / 2, 12, STR_TITLE_SINGLEPLAYER_MENU, MIDDLE, TOP, fnt_button
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
		objects.emplace_back(new Border(0, 0, WIDTH, HEIGHT, false));
		objects.emplace_back(new Text(
			WIDTH / 2, 12, STR_TITLE_MULTIPLAYER, MIDDLE, TOP, fnt_button
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
		objects.emplace_back(new Border(0, 0, WIDTH, HEIGHT, false));
		objects.emplace_back(new Text(
			WIDTH / 2, 12, STR_TITLE_SCENARIO_EDITOR, MIDDLE, TOP, fnt_button
		));
		objects.emplace_back(new Button(779, 4, 795 - 779, 16, STR_EXIT, true));

		group.add(0, 212 - 222, STR_BTN_SCENARIO_CREATE);
		group.add(0, 275 - 222, STR_BTN_SCENARIO_EDIT);
		group.add(0, 337 - 222, STR_BTN_SCENARIO_CAMPAIGN);
		group.add(0, 400 - 222, STR_BTN_SCENARIO_CANCEL);
	}

	void button_group_activate(unsigned id) override final {
		switch (id) {
		case 3:
			stop = 1;
			break;
		}
	}

	void button_activate(unsigned) override final {
		running = 0;
	}
};

class MenuMain final : public Menu {
public:
	MenuMain() : Menu(STR_TITLE_MAIN, false) {
		objects.emplace_back(bkg = new Background(DRS_BACKGROUND_MAIN, 0, 0));
		objects.emplace_back(new Border(0, 0, WIDTH, HEIGHT, false));

		group.add(0, 0, STR_BTN_SINGLEPLAYER);
		group.add(0, 285 - 222, STR_BTN_MULTIPLAYER);
		group.add(0, 347 - 222, STR_BTN_HELP);
		group.add(0, 410 - 222, STR_BTN_EDIT);
		group.add(0, 472 - 222, STR_BTN_EXIT);

		// FIXME (tm) gets truncated by resource handling in res.h (ascii, unicode stuff)
		objects.emplace_back(new Text(WIDTH / 2, 542, STR_MAIN_COPY1, CENTER));
		// FIXME (copy) and (p) before this line
		objects.emplace_back(new Text(WIDTH / 2, 561, STR_MAIN_COPY2, CENTER));
		objects.emplace_back(new Text(WIDTH / 2, 578, STR_MAIN_COPY3, CENTER));

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
		case 2: {
			char path[FS_BUFSZ], buf[FS_BUFSZ];
			fs_cdrom_path(path, sizeof path, "readme.doc");
			snprintf(buf, sizeof buf, "xdg-open \"%s\"", path);
			system(buf);
			break;
		}
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
