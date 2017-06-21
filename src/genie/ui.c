#include "ui.h"
#include <err.h>
#include <assert.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "prompt.h"
#include "gfx.h"

struct genie_ui genie_ui = {
	.game_title = "???",
	.width = 640, .height = 480,
};

#define UI_INIT 1
#define UI_INIT_SDL 2

static unsigned ui_init = 0;

void genie_ui_free(struct genie_ui *ui)
{
	if (!ui_init)
		return;

	ui_init &= ~UI_INIT;

	if (ui->gl) {
		SDL_GL_DeleteContext(ui->gl);
		ui->gl = NULL;
	}
	if (ui->win) {
		SDL_DestroyWindow(ui->win);
		ui->win = NULL;
	}

	SDL_Quit();

	ui_init &= ~UI_INIT_SDL;

	if (ui_init)
		warnx(
			"%s: expected state to be zero, but got %d",
			__func__, ui_init
		);
}

static int genie_ui_sdl_init(struct genie_ui *ui)
{
	int error = 0;

	if (SDL_Init(SDL_INIT_VIDEO))
		goto sdl_error;

	if (SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1)) {
sdl_error:
		show_error("SDL failed", SDL_GetError());
		error = GENIE_UI_ERR_SDL;
		goto fail;
	}

	ui->win = SDL_CreateWindow("AoE", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ui->width, ui->height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	if (!ui->win) {
		show_error("Engine failed", "No window");
		goto fail;
	}

	ui->gl = SDL_GL_CreateContext(ui->win);
	if (!ui->gl) {
		show_error(
		"Engine failed",
		"The graphics card could not be initialized because OpenGL\n"
		"drivers could not be loaded. Make sure your graphics card\n"
		"supports OpenGL 1.1 or better."
		);
		goto fail;
	}

	error = 0;
fail:
	return error;
}

int genie_ui_init(struct genie_ui *ui)
{
	int error = 0;

	if (ui_init)
		return 0;

	ui_init = UI_INIT;

	error = genie_ui_sdl_init(ui);
	if (error)
		goto fail;

	error = 0;
fail:
	return error;
}

int genie_ui_is_visible(const struct genie_ui *ui)
{
	return ui->win != NULL && ui->gl != NULL;
}

static void genie_ui_update(struct genie_ui *ui)
{
	SDL_GL_SwapWindow(ui->win);
}

void genie_ui_display(struct genie_ui *ui)
{
	genie_gfx_clear_screen(0, 0, 0, 0);
	genie_gfx_setup_ortho(ui->width, ui->height);

	glColor3f(1, 1, 1);
	genie_gfx_draw_text(
		0, 0, "This is going to be the new main executable\n"
		"There is nothing else to see here yet..."
	);

	genie_ui_update(ui);
}
