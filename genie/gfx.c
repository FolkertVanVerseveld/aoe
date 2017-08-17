/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "gfx.h"
#include "prompt.h"
#include "ttf.h"
#include <err.h>
#include <ctype.h>
#include <stdlib.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_opengl.h>

#define GFX_INIT 1
#define GFX_INIT_IMG 2
#define GFX_INIT_FONT 4
#define GFX_INIT_TTF 8

/* The default font texture */
extern unsigned char *font_png_start, *font_png_end;

static unsigned gfx_init = 0;
static GLuint font_tex = 0;

struct ttf_text {
	GLuint tex;
	int w, h;
};

static struct gfx_cache {
	struct ttf_text data[GENIE_GFX_CACHE_SIZE];
	unsigned count;
	int init;
} gfx_cache;

static void gfx_cache_init(struct gfx_cache *this)
{
	this->count = 0;
	this->init = 0;
}

static int gfx_cache_create(struct gfx_cache *this)
{
	GLuint tex[GENIE_GFX_CACHE_SIZE];
	if (this->init)
		return 0;
	glGenTextures(GENIE_GFX_CACHE_SIZE, tex);
	for (unsigned i = 0, n = GENIE_GFX_CACHE_SIZE; i < n; ++i)
		this->data[i].tex = tex[i];
	this->count = 0;
	this->init = 1;
	return 0;
}

static void gfx_cache_free(struct gfx_cache *this)
{
	GLuint tex[GENIE_GFX_CACHE_SIZE];
	if (!this->init)
		return;
	this->init = 0;
	for (unsigned i = 0, n = GENIE_GFX_CACHE_SIZE; i < n; ++i)
		tex[i] = this->data[i].tex;
	glDeleteTextures(GENIE_GFX_CACHE_SIZE, tex);
	gfx_cache_init(this);
}

static void gfx_cache_clear(struct gfx_cache *this)
{
	this->count = 0;
}

static void map_tex(SDL_Surface *surf, GLuint tex);

static unsigned gfx_cache_add(struct gfx_cache *this, SDL_Surface *surf)
{
	if (this->count >= GENIE_GFX_CACHE_SIZE) {
		show_error("Fatal error", "Graphics cache is full");
		exit(1);
	}
	unsigned slot = this->count++;

	struct ttf_text *text = &this->data[slot];
	map_tex(surf, text->tex);
	text->w = surf->w;
	text->h = surf->h;

	return slot;
}

void genie_gfx_free(void)
{
	if (!gfx_init)
		return;

	gfx_init &= ~GFX_INIT;

	gfx_cache_free(&gfx_cache);

	if (gfx_init & GFX_INIT_TTF) {
		genie_ttf_free();
		gfx_init &= ~GFX_INIT_TTF;
	}

	if (gfx_init & GFX_INIT_IMG) {
		IMG_Quit();
		gfx_init &= ~GFX_INIT_IMG;
	}

	if (gfx_init)
		warnx(
			"%s: expected state to be zero, but got %d",
			__func__, gfx_init
		);
}

static void map_tex(SDL_Surface *surf, GLuint tex)
{
	GLint internal;
	GLenum format;
	void *pixels;
	uint8_t *rgba = NULL;

	pixels = surf->pixels;

	glPixelTransferi(GL_MAP_COLOR, GL_FALSE);
	glBindTexture(GL_TEXTURE_2D, tex);

	if (surf->format->palette) {
		int ww;
		size_t size;
		uint8_t *data;
		SDL_Color *pal;
		int y, x;

		format = internal = GL_RGB;

		ww = 4 * ((surf->w - 1) / 4 + 1);

		if (surf->format->BytesPerPixel != 1) {
			show_error("Fatal error", "Unknown color palette format");
			exit(1);
		}

		size = surf->w * surf->h * 4 + 4;
		rgba = malloc(size);

		if (!rgba)
			show_oem(1);

		memset(rgba, 0xff, size);
		data = pixels;
		pal = surf->format->palette->colors;

		for (y = 0; y < surf->h; ++y)
			for (x = 0; x < surf->w; ++x) {
				uint8_t i;

				i = data[y * ww + x];

				rgba[4 * (y * surf->w + x) + 0] = pal[i].r;
				rgba[4 * (y * surf->w + x) + 1] = pal[i].g;
				rgba[4 * (y * surf->w + x) + 2] = pal[i].b;
				rgba[4 * (y * surf->w + x) + 3] = pal[i].a;
			}

		pixels = rgba;
		format = internal = GL_RGBA;
	} else {
		internal = (!surf->format->Amask || surf->format->BitsPerPixel == 24) ? GL_RGB : GL_RGBA;
		format = internal;
	}

	glTexImage2D(
		GL_TEXTURE_2D, 0,
		internal, surf->w, surf->h,
		0, format, GL_UNSIGNED_BYTE, pixels
	);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	if (rgba)
		free(rgba);
}

int gfx_init_font(void)
{
	SDL_Surface *surf = NULL;
	SDL_RWops *fontdata = NULL;
	size_t fontsz;
	int error = 1;

	fontsz = (size_t)(font_png_end - font_png_start);
	fontdata = SDL_RWFromConstMem(font_png_start, fontsz);

	if (!fontdata) {
		show_error("Graphics failed", "No font found");
		goto fail;
	}

	surf = IMG_LoadPNG_RW(fontdata);

	if (!surf) {
		char str[1024];
		snprintf(str, sizeof str, "Error occurred while trying to load font:\n%s", IMG_GetError());
		show_error("Graphics failed", str);
		goto fail;
	}

	glGenTextures(1, &font_tex);
	map_tex(surf, font_tex);

	error = 0;
fail:
	if (surf)
		SDL_FreeSurface(surf);
	if (fontdata)
		SDL_RWclose(fontdata);

	return error;
}

int genie_gfx_init(void)
{
	int error = 1, img_flags = IMG_INIT_PNG;

	if (gfx_init) {
		warnx("%s: already initialized", __func__);
		return 0;
	}
	gfx_init = GFX_INIT;
	gfx_cache_init(&gfx_cache);

	if ((IMG_Init(img_flags) & img_flags) != img_flags) {
		char str[1024];
		const char *img_error;

		img_error = IMG_GetError();

		if (!img_error)
			img_error = "Unknown internal error";

		snprintf(str, sizeof str,
			"Cannot setup graphics subsystem. Either PNG support is not\n"
			"compiled in SDL2_image or SDL2_image could not be found. Make\n"
			"sure your distribution ships SDL2_image with PNG support.\n"
			"Diagnostic error message: %s\n",
			img_error
		);
		show_error("Graphics failed", str);

		goto fail;
	}

	gfx_init |= GFX_INIT_IMG;

	error = gfx_init_font();
	if (error)
		goto fail;
	error = genie_ttf_init();
	if (error)
		goto fail;
	gfx_cache_create(&gfx_cache);

	error = 0;
fail:
	return error;
}

void genie_gfx_setup_ortho(GLdouble width, GLdouble height)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width, height, 0, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void genie_gfx_clear_screen(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	glClearColor(red, green, blue, alpha);
	glClear(GL_COLOR_BUFFER_BIT);
}

void gfx_draw_text(int x, int y, const char *str)
{
	GLfloat tx0, tx1, ty0, ty1;
	GLfloat vx0, vx1, vy0, vy1;
	unsigned gx, gy;
	int xp, ch;

	for (xp = x; *str; ++str, x += GENIE_GLYPH_WIDTH) {
		ch = *str;

		while (ch == '\n') {
			x = xp;
			y += GENIE_GLYPH_HEIGHT;
			ch = *++str;
		}

		if (!ch)
			break;

		gy = ch / GENIE_FONT_TILE_WIDTH;
		gx = ch % GENIE_FONT_TILE_WIDTH;

		tx0 = 1.0f * gx / GENIE_FONT_TILE_WIDTH;
		tx1 = 1.0f * (gx + 1) / GENIE_FONT_TILE_WIDTH;
		ty0 = 1.0f * gy / GENIE_FONT_TILE_HEIGHT;
		ty1 = 1.0f * (gy + 1) / GENIE_FONT_TILE_HEIGHT;

		vx0 = x; vx1 = x + GENIE_GLYPH_WIDTH;
		vy0 = y; vy1 = y + GENIE_GLYPH_HEIGHT;

		glTexCoord2f(tx0, ty0); glVertex2f(vx0, vy0);
		glTexCoord2f(tx1, ty0); glVertex2f(vx1, vy0);
		glTexCoord2f(tx1, ty1); glVertex2f(vx1, vy1);
		glTexCoord2f(tx0, ty1); glVertex2f(vx0, vy1);
	}
}

void genie_gfx_draw_text(int x, int y, const char *str)
{
	glBindTexture(GL_TEXTURE_2D, font_tex);
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
		gfx_draw_text(x, y, str);
	glEnd();
	glDisable(GL_TEXTURE_2D);
}

void genie_gfx_cache_clear(void)
{
	gfx_cache_clear(&gfx_cache);
}

unsigned genie_gfx_cache_text(unsigned ttf_id, const char *text)
{
	SDL_Color color = {255, 255, 255, 255};
	SDL_Surface *surf = genie_ttf_render_solid(ttf_id, text, color);
	unsigned slot = gfx_cache_add(&gfx_cache, surf);
	SDL_FreeSurface(surf);
	return slot;
}

void genie_gfx_put_text(unsigned slot, int x, int y, unsigned halign, unsigned valign)
{
	const struct ttf_text *text = &gfx_cache.data[slot];
	int w = text->w, h = text->h;
	switch (halign) {
	case GENIE_HA_LEFT:
		break;
	case GENIE_HA_CENTER:
		x -= w / 2;
		break;
	case GENIE_HA_RIGHT:
		x -= w;
		break;
	}
	switch (valign) {
	case GENIE_VA_TOP:
		break;
	case GENIE_VA_MIDDLE:
		y -= h / 2;
		break;
	case GENIE_VA_BOTTOM:
		y -= h;
		break;
	}
	glBindTexture(GL_TEXTURE_2D, text->tex);
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
		glTexCoord2f(0, 0); glVertex2f(x, y);
		glTexCoord2f(1, 0); glVertex2f(x + w, y);
		glTexCoord2f(1, 1); glVertex2f(x + w, y + h);
		glTexCoord2f(0, 1); glVertex2f(x, y + h);
	glEnd();
	glDisable(GL_TEXTURE_2D);
}
