/* Copyright 2016-2018 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

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

#include "game.hpp"

extern struct pe_lib lib_lang;

/* load c-string from language dll and wrap into c++ string */
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
	void keydown(SDL_KeyboardEvent *event);
	void keyup(SDL_KeyboardEvent *event);

	/* Push new menu onto navigation stack */
	void go_to(Menu *menu);

	void dirty() {
		state |= DIRTY;
	}

	void display();
	void dispose();
private:
	void update();
} ui_state;

/* Custom renderer */
class Renderer final {
	SDL_Surface *capture;
	SDL_Texture *tex;
	void *pixels;
public:
	SDL_Renderer *renderer;
	unsigned shade;

	Renderer() : capture(NULL), tex(NULL), renderer(NULL) {
		if (!(pixels = malloc(WIDTH * HEIGHT * 3)))
			panic("Out of memory");
	}

	~Renderer() {
		if (pixels)
			free(pixels);
		if (capture)
			SDL_FreeSurface(capture);
	}

	void col(int grayvalue) {
		col(grayvalue, grayvalue, grayvalue);
	}

	void col(int r, int g, int b, int a = SDL_ALPHA_OPAQUE) {
		SDL_SetRenderDrawColor(renderer, r, g, b, a);
	}

	void col(const SDL_Color &col) {
		SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, col.a);
	}

	void read_screen() {
		if (capture)
			SDL_FreeSurface(capture);
		// FIXME support big endian byte order
		if (!(capture = SDL_CreateRGBSurface(0, WIDTH, HEIGHT, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000)))
			panic("read_screen");

		SDL_RenderReadPixels(renderer, NULL, SDL_PIXELFORMAT_ARGB8888, capture->pixels, capture->pitch);

		if (!(tex = SDL_CreateTextureFromSurface(renderer, capture)))
			panic("read_screen");
	}

	void dump_screen() {
		SDL_Rect pos = {0, 0, WIDTH, HEIGHT};
		SDL_RenderCopy(renderer, tex, NULL, &pos);
	}

	void clear() {
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(renderer);
	}
} canvas;

bool point_in_rect(int x, int y, int w, int h, int px, int py)
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

/** Text horizontal/vertical alignment */
enum TextAlign {
	LEFT = 0, TOP = 0,
	CENTER = 1, MIDDLE = 1,
	RIGHT = 2, BOTTOM = 2
};

const SDL_Color col_default = {255, 208, 157, SDL_ALPHA_OPAQUE};
const SDL_Color col_focus = {255, 255, 0, SDL_ALPHA_OPAQUE};

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
		, TTF_Font *fnt=fnt_default
		, SDL_Color col=col_default)
		: UI(x, y), str(load_string(id))
	{
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
	void draw() const {
		SDL_Rect pos = {x - 1, y + 1, (int)w, (int)h};
		// draw shadow
		SDL_SetTextureColorMod(tex, 0, 0, 0);
		SDL_RenderCopy(renderer, tex, NULL, &pos);
		++pos.x;
		--pos.y;
		SDL_SetTextureColorMod(tex, 255, 255, 255);
		SDL_RenderCopy(renderer, tex, NULL, &pos);
	}
};

SDL_Color border_cols[6] = {};

// FIXME more color schemes
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
			SDL_Color col = {0, 0, 0, (Uint8)(255 - ((shade != -1 ? shade : canvas.shade) * 255 / 100))};
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

class Palette final {
public:
	SDL_Palette *pal;

	Palette() : pal(NULL) {}
	Palette(unsigned id) : pal(NULL) { open(id); }

	~Palette() {
		if (pal)
			SDL_FreePalette(pal);
	}

	void open(unsigned id) {
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

	void set_border_color(unsigned id, unsigned col_id) const {
		border_cols[id].r = pal->colors[col_id].r;
		border_cols[id].g = pal->colors[col_id].g;
		border_cols[id].b = pal->colors[col_id].b;
		border_cols[id].a = SDL_ALPHA_OPAQUE;
	}
};

unsigned cmd_or_next(const unsigned char **cmd, unsigned n)
{
	const unsigned char *ptr = *cmd;
	unsigned v = *ptr >> n;
	if (!v)
		v = *(++ptr);
	*cmd = ptr;
	return v;
}

class Image final {
public:
	SDL_Surface *surface;
	SDL_Texture *texture;

	Image() : surface(NULL), texture(NULL) {}

	void load(Palette *pal, const void *data, const struct slp_frame_info *frame)
	{
		dbgf("dimensions: %u x %u\n", frame->width, frame->height);
		dbgf("hostpot: %u,%u\n", frame->hotspot_x, frame->hotspot_y);
		dbgf("command table offset: %X\n", frame->cmd_table_offset);
		dbgf("outline table offset: %X\n", frame->outline_table_offset);

		// FIXME big endian support
		surface = SDL_CreateRGBSurface(0, frame->width, frame->height, 8, 0, 0, 0, 0);

		if (!surface || SDL_SetSurfacePalette(surface, pal->pal))
			panicf("Cannot create image: %s\n", SDL_GetError());
		if (surface->format->format != SDL_PIXELFORMAT_INDEX8)
			panicf("Unexpected image format: %s\n", SDL_GetPixelFormatName(surface->format->format));

		// TODO read pixel data
		// fill with random data for now
		unsigned char *pixels = (unsigned char*)surface->pixels;
		for (int y = 0, h = frame->height, p = surface->pitch; y < h; ++y)
			for (int x = 0, w = frame->width; x < w; ++x)
				pixels[y * p + x] = 0;

		const struct slp_frame_row_edge *edge =
			(const struct slp_frame_row_edge*)
				((char*)data + frame->outline_table_offset);
		const unsigned char *cmd = (const unsigned char*)
			//((char*)data + frame->cmd_table_offset);
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
				unsigned command, higher_nibble, count;

				command = *cmd & 0x0f;
				higher_nibble = *cmd & 0xf0;

				// TODO figure out if lesser skip behaves different compared to aoe2

				switch (*cmd) {
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
				case 0x07:
					count = cmd_or_next(&cmd, 4);
					//dbgf("fill: %u pixels\n", count);
					for (++cmd; count; --count) {
						//dbgf(" %x", (unsigned)(*cmd) & 0xff);
						pixels[y * p + x++] = *cmd;
					}
					//dbgs("");
					break;
#if 0
// source: openage/openage/convert/slp.pyx:385
// fill command
// draw 'count' pixels with color of next byte

cpack = self.cmd_or_next(cmd, 4, dpos)
dpos = cpack.dpos

dpos += 1
color = self.get_byte_at(dpos)

for _ in range(cpack.count):
row_data.push_back(pixel(color_standard, color))
#endif
				case 0x0f:
					//dbgs("break");
					i = w;
					break;
				default:
					dbgf("unknown cmd: %X, %X, %X\n", *cmd, command, higher_nibble);
					break;
				}
			}
		}
		putchar('\n');

		if (!(texture = SDL_CreateTextureFromSurface(renderer, surface)))
			panicf("Cannot create texture: %s\n", SDL_GetError());
	}

	~Image() {
		if (texture)
			SDL_DestroyTexture(texture);
		if (surface)
			SDL_FreeSurface(surface);
	}

	void draw(int x, int y) const {
		if (!texture)
			return;

		SDL_Rect pos = {x, y, surface->w, surface->h};
		SDL_RenderCopy(renderer, texture, NULL, &pos);
	}
};

/** SLP image wrapper. */
class AnimationTexture final {
	struct slp image;
	Image *images;

public:
	AnimationTexture() : images(NULL) {}
	AnimationTexture(Palette *pal, unsigned id) : images(NULL) { open(pal, id); }

	~AnimationTexture() {
		if (images)
			delete[] images;
	}

	void open(Palette *pal, unsigned id) {
		size_t n;
		const void *data = drs_get_item(DT_SLP, id, &n);

		slp_read(&image, data);

		if (memcmp(image.hdr->version, "2.0N", 4))
			panicf("Bad Animation Texture id %u", id);

		dbgf("slp info %u:\n", id);
		dbgf("frame count: %u\n", image.hdr->frame_count);

		images = new Image[image.hdr->frame_count];

		// FIXME parse all frames
		for (size_t i = 0, n = image.hdr->frame_count; i < n; ++i) {
			dbgf("frame %zu:\n", i);
			struct slp_frame_info *frame = &image.info[i];
			images[i].load(pal, data, frame);
		}
	}

	void draw(int x, int y, unsigned index) const {
		images[index % image.hdr->frame_count].draw(x, y);
	}
};

class Background final : public UI {
	unsigned id;
	Palette palette;
	AnimationTexture animation;
	unsigned bevel_col[6];
public:
	Background(unsigned id, int x, int y, unsigned w = 1, unsigned h = 1)
		: UI(x, y, w, h), id(id), palette(), animation()
	{
		size_t n;
		const char *data = (const char*)drs_get_item(DT_BINARY, id, &n);

		char bkg_name1[16], bkg_name2[16], bkg_name3[16];
		unsigned bkg_id[3];
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
			"background1_files %15s none %u -1\n"
			"background2_files %15s none %u -1\n"
			"background3_files %15s none %u -1\n"
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

		animation.open(&palette, bkg_id[1]);
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
		animation.draw(x, y, 0);
	}
};

class Button final : public Border {
	Text text, text_focus;
public:
	bool focus;
	bool down;
	bool play_sfx;

	Button(int x, int y, unsigned w, unsigned h, unsigned id, bool def_fnt=false, bool play_sfx=true)
		: Border(x, y, w, h)
		, text(x + w / 2, y + h / 2, id, CENTER, MIDDLE, def_fnt ? fnt_default : fnt_button)
		, text_focus(x + w / 2, y + h / 2, id, CENTER, MIDDLE, def_fnt ? fnt_default : fnt_button, col_focus)
		, focus(false), down(false), play_sfx(play_sfx)
	{
	}

	Button(int x, int y, unsigned w, unsigned h, const std::string &str, bool def_fnt=false, bool play_sfx=true)
		: Border(x, y, w, h)
		, text(x + w / 2, y + h / 2, str, CENTER, MIDDLE, def_fnt ? fnt_default : fnt_button)
		, text_focus(x + w / 2, y + h / 2, str, CENTER, MIDDLE, def_fnt ? fnt_default : fnt_button, col_focus)
		, focus(false), down(false), play_sfx(play_sfx)
	{
	}

	/* Process mouse event if it has been clicked. */
	bool mousedown(SDL_MouseButtonEvent *event) {
		bool old_down = down;

		focus = down = contains(event->x, event->y);

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
		Border::draw(down);
		if (focus)
			text_focus.draw();
		else
			text.draw();
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

	virtual void draw() const {
		for (auto x : objects)
			x.get()->draw();

		group.update();
		group.draw();
	}

	void keydown(SDL_KeyboardEvent *event) {
		unsigned virt = event->keysym.sym;

		if (virt == SDLK_DOWN)
			group.ror();
		else if (virt == SDLK_UP)
			group.rol();
		else if (virt == ' ')
			group.down();
	}

	void keyup(SDL_KeyboardEvent *event) {
		unsigned virt = event->keysym.sym;

		if (virt == ' ') {
			sfx_play(SFX_BUTTON4);
			group.up();
			button_group_activate(group.focus);
		}
	}

	void mousedown(SDL_MouseButtonEvent *event) {
		group.mousedown(event);

		for (auto x : objects) {
			Button *btn = dynamic_cast<Button*>(x.get());
			if (btn)
				if (btn->mousedown(event))
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

		objects.emplace_back(new Border(12, 106, 787 - 12, 518 - 106, 0));

		group.add(250, 551, STR_BTN_BACK);

		unsigned i = 1, step = tl_height / players.size();

		for (auto x : players) {
			auto p = x.get();
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

		canvas.clear();
	}

	virtual void button_group_activate(unsigned) override final {
		stop = 1;
	}

	void button_activate(unsigned id) override final {
		switch (id) {
		case 2: running = 0; break;
		}
	}

	virtual void draw() const override {
		Menu::draw();

		unsigned i = 0, step = (tl_height + 1) / players.size();
		for (unsigned n = players.size(); i < n; ++i) {
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
		//objects.emplace_back(new Text(485, 137, 639 - 485, 162 - 137, STR_SURVIVAL, RIGHT, TOP, fnt_default));
		//objects.emplace_back(new Text(552, 162, 713 - 552, 187 - 162, STR_WONDER, RIGHT, TOP, fnt_default));
		//objects.emplace_back(new Text(617, 187, 781 - 617, 212 - 187, STR_TOTAL_SCORE, RIGHT, TOP, fnt_default));
		objects.emplace_back(new Text(633, 140, STR_SURVIVAL, RIGHT, TOP, fnt_default));
		objects.emplace_back(new Text(710, 165, STR_WONDER, RIGHT, TOP, fnt_default));
		objects.emplace_back(new Text(778, 190, STR_TOTAL_SCORE, RIGHT, TOP, fnt_default));

		unsigned y = 225, nr = 0;

		for (auto x : players) {
			auto p = x.get();

			objects.emplace_back(new Text(31, y, p->name, LEFT, TOP, fnt_default, col_players[nr % MAX_PLAYER_COUNT]));

			y += 263 - 225;
			++nr;
		}

		if (type)
			mus_play(MUS_DEFEAT);
	}

	virtual void button_group_activate(unsigned id) override final {
		switch (id) {
		case 0:
			if (type) {
				stop = 5;
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
		objects.emplace_back(new Border(100, 105, 700 - 100, 495 - 105));

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

		objects.emplace_back(new Border(200, 98, 600 - 200, 503 - 98));
	}

	void button_group_activate(unsigned id) override final {
		switch (id) {
		case 0: ui_state.go_to(new MenuAchievements(1)); break;
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
public:
	MenuGame()
		: Menu(STR_TITLE_MAIN, 0, 0, 728 - 620, 18, false)
		, palette(), menu_bar()
	{
		group.add(728, 0, STR_BTN_MENU, WIDTH - 728, 18, true);
		group.add(620, 0, STR_BTN_DIPLOMACY, 728 - 620, 18, true);

		objects.emplace_back(new Text(WIDTH / 2, 3, STR_AGE_STONE, MIDDLE, TOP));
		objects.emplace_back(new Text(WIDTH / 2, HEIGHT / 2, STR_PAUSED, MIDDLE, CENTER, fnt_button));

		const Player *you = players[0].get();

		char buf[32];
		int x = 4;
		snprintf(buf, sizeof buf, "F: %u", you->resources.food);
		objects.emplace_back(new Text(x, 3, buf));
		x += 80;
		snprintf(buf, sizeof buf, "W: %u", you->resources.wood);
		objects.emplace_back(new Text(x, 3, buf));
		x += 80;
		snprintf(buf, sizeof buf, "G: %u", you->resources.gold);
		objects.emplace_back(new Text(x, 3, buf));
		x += 80;
		snprintf(buf, sizeof buf, "S: %u", you->resources.stone);
		objects.emplace_back(new Text(x, 3, buf));

		objects.emplace_back(new Button(765, 482, 795 - 765, 512 - 482, STR_BTN_SCORE, true));
		objects.emplace_back(new Button(765, 564, 795 - 765, 594 - 564, "?", true));

		canvas.clear();

		palette.open(DRS_MAIN_PALETTE);
		menu_bar.open(&palette, DRS_MENU_BAR);

		mus_play(MUS_GAME);
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
		switch (id) {
		case 0: break;
		case 1: break;
		}
	}

	void draw() const override final {
		menu_bar.draw(0, 0, 0);
		menu_bar.draw(0, HEIGHT - 126, 1);
		Menu::draw();
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
		players.clear();
		players.emplace_back(new PlayerHuman("You"));
		players.emplace_back(new PlayerComputer());
		players.emplace_back(new PlayerComputer());
		players.emplace_back(new PlayerComputer());

		char str_count[2] = "0";
		str_count[0] += players.size();

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
}

/* Wrappers for ui_state */
void display() { ui_state.display(); }

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

void UI_State::display()
{
	if ((state & DIRTY) && !navigation.empty()) {
		// Draw stuff
		navigation.top().get()->draw();
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

	auto menu = navigation.top().get();
	menu->keyup(event);

	update();
}

void UI_State::update()
{
	for (unsigned n = navigation.top().get()->stop; n; --n) {
		if (navigation.empty())
			return;
		navigation.pop();
	}
	navigation.top().get()->restore();
}
