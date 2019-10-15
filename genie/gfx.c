/* Copyright 2018-2019 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include <genie/gfx.h>

#include <genie/def.h>

#include <stdio.h>
#include <SDL2/SDL_image.h>

#include "fnt_data.h"

struct gfx_cfg gfx_cfg = {0, 0, 0, 800, 600};

#define BUFSZ 4096

extern const unsigned char _binary_fnt_default_png_start[];
extern const unsigned char _binary_fnt_default_png_end[];
extern const unsigned char _binary_fnt_default_bold_png_start[];
extern const unsigned char _binary_fnt_default_bold_png_end[];
extern const unsigned char _binary_fnt_button_png_start[];
extern const unsigned char _binary_fnt_button_png_end[];
extern const unsigned char _binary_fnt_large_png_start[];
extern const unsigned char _binary_fnt_large_png_end[];

SDL_Texture *fnt_tex_default, *fnt_tex_default_bold, *fnt_tex_button, *fnt_tex_large;

extern SDL_Renderer *renderer;

static void ge_gfx_load_font(SDL_Texture **dst, const void *start, const void *end)
{
	SDL_RWops *mem;
	SDL_Surface *surf;
	SDL_Texture *tex;

	mem = SDL_RWFromConstMem(start, (int)(end - start));
	if (!mem)
		panic("Could not load font from memory");

	if (!(surf = IMG_Load_RW(mem, 1)) || !(tex = SDL_CreateTextureFromSurface(renderer, surf)))
		panic("Corrupt font data");

	*dst = tex;
}

void gfx_init(void)
{
	char buf[BUFSZ];

	if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
		panic("Could not initialize image subsystem");

	ge_gfx_load_font(&fnt_tex_default, _binary_fnt_default_png_start, _binary_fnt_default_png_end);
	ge_gfx_load_font(&fnt_tex_default_bold, _binary_fnt_default_bold_png_start, _binary_fnt_default_bold_png_end);
	ge_gfx_load_font(&fnt_tex_button, _binary_fnt_button_png_start, _binary_fnt_button_png_end);
	ge_gfx_load_font(&fnt_tex_large, _binary_fnt_large_png_start, _binary_fnt_large_png_end);
}

void gfx_free(void)
{
	SDL_DestroyTexture(fnt_tex_large);
	SDL_DestroyTexture(fnt_tex_button);
	SDL_DestroyTexture(fnt_tex_default_bold);
	SDL_DestroyTexture(fnt_tex_default);

	IMG_Quit();
}

int gfx_get_textlen_bounds(struct SDL_Texture *font, SDL_Rect *bounds, const char *text, unsigned count)
{
	// FIXME use pos->w and pos->h to clip glyphs if out of bounds
	SDL_Rect ren_pos, tex_pos;
	int x = 0, y = 0;
	int tx = x, ty = y;
	const struct fnt_tm *tm;
	const struct fnt_cs *cs;

	if (font == fnt_tex_default) { tm = &fnt_tm_default; cs = &fnt_cs_default; }
	else if (font == fnt_tex_default_bold) { tm = &fnt_tm_default_bold; cs = &fnt_cs_default_bold; }
	else if (font == fnt_tex_button) { tm = &fnt_tm_button; cs = &fnt_cs_button; }
	else if (font == fnt_tex_large) { tm = &fnt_tm_large; cs = &fnt_cs_large; }
	else { panic("bad font"); }

	int max_w = 0, max_h = 0;

	for (const unsigned char *ptr = (const unsigned char*)text; count; ++ptr, --count) {
		int gpos, gx, gy, gw, gh;
		unsigned ch = *ptr;

		// treat invalid characters as break
		if (ch < tm->tm_chrfst || ch > tm->tm_chrend)
			ch = tm->tm_chrbrk;

		if (ch == '\r')
			continue;

		if (ch == '\n') {
			tx = x;
			ty += tm->tm_height;
		}

		gw = cs->data[2 * ch + 0];
		gh = cs->data[2 * ch + 1];
		gpos = ch - tm->tm_chrfst;
		gy = gpos / 16;
		gx = gpos % 16;

		if (tx + gw >= max_w)
			max_w = tx + gw;
		if (ty + gh >= max_h)
			max_h = ty + gh;

		tx += gw;
	}

	bounds->w = max_w;
	bounds->h = max_h;
	return 0;
}

void gfx_draw_textlen(struct SDL_Texture *font, const SDL_Rect *pos, const char *text, unsigned count)
{
	// FIXME use pos->w and pos->h to clip glyphs if out of bounds
	SDL_Rect ren_pos, tex_pos;
	int x = pos->x, y = pos->y;
	int tx = x, ty = y;
	const struct fnt_tm *tm;
	const struct fnt_cs *cs;

	if (font == fnt_tex_default) { tm = &fnt_tm_default; cs = &fnt_cs_default; }
	else if (font == fnt_tex_default_bold) { tm = &fnt_tm_default_bold; cs = &fnt_cs_default_bold; }
	else if (font == fnt_tex_button) { tm = &fnt_tm_button; cs = &fnt_cs_button; }
	else if (font == fnt_tex_large) { tm = &fnt_tm_large; cs = &fnt_cs_large; }
	else { panic("bad font"); }

	for (const unsigned char *ptr = (const unsigned char*)text; count; ++ptr, --count) {
		int gpos, gx, gy, gw, gh;
		unsigned ch = *ptr;

		// treat invalid characters as break
		if (ch < tm->tm_chrfst || ch > tm->tm_chrend)
			ch = tm->tm_chrbrk;

		if (ch == '\r')
			continue;

		if (ch == '\n') {
			tx = x;
			ty += tm->tm_height;
		}

		gw = cs->data[2 * ch + 0];
		gh = cs->data[2 * ch + 1];
		gpos = ch - tm->tm_chrfst;
		gy = gpos / 16;
		gx = gpos % 16;

		ren_pos.x = tx;
		ren_pos.y = ty;
		ren_pos.w = gw;
		ren_pos.h = gh;

		tex_pos.x = gx * cs->max_w;
		tex_pos.y = gy * cs->max_h;
		tex_pos.w = gw;
		tex_pos.h = gh;

		SDL_RenderCopy(renderer, font, &tex_pos, &ren_pos);

		tx += gw;
	}
}

void gfx_draw_textlen_shadow(struct SDL_Texture *tex, const SDL_Color *fg, const SDL_Color *bg, const SDL_Rect *pos, const char *text, unsigned count)
{
	SDL_Rect pos2 = {pos->x - 1, pos->y + 1, pos->w, pos->h};
	SDL_SetTextureColorMod(tex, bg->r, bg->g, bg->b);
	gfx_draw_textlen(tex, &pos2, text, count);
	SDL_SetTextureColorMod(tex, fg->r, fg->g, fg->b);
	gfx_draw_textlen(tex, pos, text, count);
}

void gfx_draw_textlen_shadow_ext(struct SDL_Texture *tex, const SDL_Color *fg, const SDL_Color *bg, const SDL_Rect *pos, const char *text, unsigned count, enum Halign ha, enum Valign va)
{
	SDL_Rect bounds;
	gfx_get_textlen_bounds(tex, &bounds, text, count);

	SDL_Rect target = *pos;

	switch (ha) {
	case LEFT: break;
	case CENTER: target.x -= bounds.w / 2; break;
	case RIGHT: target.x -= bounds.w; break;
	}

	switch (va) {
	case TOP: break;
	case MIDDLE: target.y -= bounds.h / 2; break;
	case BOTTOM: target.y -= bounds.h; break;
	}

	SDL_Rect pos2 = {target.x - 1, target.y + 1, target.w, target.h};
	SDL_SetTextureColorMod(tex, bg->r, bg->g, bg->b);
	gfx_draw_textlen(tex, &pos2, text, count);
	SDL_SetTextureColorMod(tex, fg->r, fg->g, fg->b);
	gfx_draw_textlen(tex, &target, text, count);
}
