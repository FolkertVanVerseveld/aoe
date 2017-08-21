/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "ui.h"

#include <err.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <signal.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include "_build.h"
#include "engine.h"
#include "game.h"
#include "gfx.h"
#include "sfx.h"
#include "ttf.h"
#include "prompt.h"
#include "string.h"

struct genie_ui genie_ui = {
	.game_title = "???",
	.width = 800, .height = 600,
	.console_show = 0,
};

#define UI_INIT 1
#define UI_INIT_SDL 2

static unsigned ui_init = 0;

static const char *civ_ages[] = {
	"Stone Age",
	"Tool Age",
	"Bronze Age",
	"Iron Age"
};

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

int genie_ui_init(struct genie_ui *ui, struct genie_game *game)
{
	int error = 0;

	if (ui_init)
		return 0;

	ui_init = UI_INIT;

	error = genie_ui_sdl_init(ui);
	if (error)
		goto fail;

	menu_nav_start.title = ui->game_title;
	genie_ui_menu_push(ui, &menu_nav_start);
	console_init(&ui->console);

	ui->menu_press = 0;
	ui->game = game;

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

static void draw_box(GLfloat x, GLfloat y, GLfloat width, GLfloat height);

static void draw_console(struct genie_ui *ui)
{
	static int tick_cursor = 0, blink_cursor = 0;

	draw_box(1, 0, ui->width - 1, ui->height);
	glColor3f(1, 1, 1);
	genie_gfx_draw_text(2 * GENIE_GLYPH_WIDTH, GENIE_GLYPH_HEIGHT,
		"Age of Empires Free Software Remake console\n"
		"Built with: " GENIE_BUILD_OPTIONS "\n"
		"Commit: " GENIE_GIT_SHA "\n"
		"Press ` to hide the console"
	);

	const struct console *c = &ui->console;
	unsigned i, j, n;
	for (i = c->screen.start, j = 0, n = c->screen.size; j < n; ++j, i = (i + 1) % GENIE_CONSOLE_ROWS)
		genie_gfx_draw_text(
			2 * GENIE_GLYPH_WIDTH,
			(6 + j) * GENIE_GLYPH_HEIGHT,
			c->screen.text[i]
		);
	j += 6;

	genie_gfx_draw_text(
		2 * GENIE_GLYPH_WIDTH,
		j * GENIE_GLYPH_HEIGHT,
		CONSOLE_PS1
	);
	genie_gfx_draw_text(
		(2 + strlen(CONSOLE_PS1)) * GENIE_GLYPH_WIDTH,
		j * GENIE_GLYPH_HEIGHT,
		ui->console.line_buffer
	);
	if (tick_cursor++ >= 10) {
		tick_cursor = 0;
		blink_cursor ^= 1;
	}
	if (blink_cursor)
		genie_gfx_draw_text(
			(2 + strlen(CONSOLE_PS1) + strlen(ui->console.line_buffer)) * GENIE_GLYPH_WIDTH,
			j * GENIE_GLYPH_HEIGHT,
			"|"
		);
}

static void draw_box(GLfloat x, GLfloat y, GLfloat width, GLfloat height)
{
	GLfloat left, right, top, bottom;

	left = x;
	right = x + width;
	top = y;
	bottom = y + height;

	glBegin(GL_LINES);
		/* outer rectangle */
		glColor3ub(145, 136, 71);
		glVertex2f(left, top + 1);
		glVertex2f(right, top + 1);
		glVertex2f(right, top + 1);
		glVertex2f(right, bottom - 1);
		glColor3ub(41, 33, 16);
		glVertex2f(left, top + 0);
		glVertex2f(left, bottom);
		glVertex2f(left, bottom);
		glVertex2f(right, bottom);
		/* margin rectangle */
		glColor3ub(129, 112, 65);
		glVertex2f(left + 1, top + 2);
		glVertex2f(right - 1, top + 2);
		glVertex2f(right - 1, top + 2);
		glVertex2f(right - 1, bottom - 2);
		glColor3ub(78, 61, 49);
		glVertex2f(left + 1, top + 1);
		glVertex2f(left + 1, bottom - 1);
		glVertex2f(left + 1, bottom - 1);
		glVertex2f(right - 1, bottom - 1);
		/* inner rectangle */
		glColor3ub(97, 78, 50);
		glVertex2f(left + 2, top + 3);
		glVertex2f(right - 2, top + 3);
		glVertex2f(right - 2, top + 3);
		glVertex2f(right - 2, bottom - 3);
		glColor3ub(107, 85, 34);
		glVertex2f(left + 2, top + 2);
		glVertex2f(left + 2, bottom - 2);
		glVertex2f(left + 2, bottom - 2);
		glVertex2f(right - 2, bottom - 2);
	glEnd();
}

static void update_menu_cache(struct genie_ui *ui)
{
	const struct menu_nav *nav = genie_ui_menu_peek(ui);
	const struct menu_list *list = nav->list;

	genie_gfx_cache_clear();

	for (unsigned i = 0, n = list->count; i < n; ++i)
		ui->slots[i] = genie_gfx_cache_text(
			GENIE_TTF_UI,
			genie_ui_button_get_text(&list->buttons[i])
		);
	ui->slots[list->count] = genie_gfx_cache_text(GENIE_TTF_UI_BOLD, nav->title);
}

static void draw_menu(struct genie_ui *ui)
{
	struct menu_nav *nav;
	const struct menu_list *list;
	unsigned i, n;

	nav = genie_ui_menu_peek(ui);

	if (!nav)
		return;
	if (nav != ui->old_top)
		update_menu_cache(ui);
	ui->old_top = nav;

	list = nav->list;
	n = list->count;

	for (i = 0; i < n; ++i) {
		const struct genie_ui_button *btn = &list->buttons[i];

		draw_box(btn->x, btn->y, btn->w, btn->h);

		if (i == nav->index)
			glColor3ub(255, 255, 0);
		else
			glColor3ub(237, 206, 186);

		genie_gfx_put_text(
			ui->slots[i], btn->x + btn->w / 2, btn->y + btn->h / 2,
			GENIE_HA_CENTER, GENIE_VA_MIDDLE
		);
	}

	glColor3f(1, 1, 1);
	genie_gfx_put_text(
		ui->slots[i], ui->width / 2, 20,
		GENIE_HA_CENTER, GENIE_VA_MIDDLE
	);
}

static void console_clear_line_buffer(struct console *c)
{
	c->line_buffer[0] = '\0';
	c->line_count = 0;
}

static const char *help_console =
	"Commands:\n"
	"  help    This help\n"
	"  info    Print statistics\n"
	"  quit    Quit game without confirmation\n"
	"  toggle  Toggle option";

static const char *help_quit =
	"quit: Quit game without confirmation\n"
	"  Does not save anything so make sure you are not doing something important!";

static const char *help_info =
	"info: Show information statistics\n"
	"  Print current state of options and statistics for testing purposes.";

static const char *help_toggle =
	"toggle option: Toggle option\n"
	"  The following options are supported:\n"
	"    border    Border around game display";

static const char *help_play = "play ID: Play sound";

static const char *help_purge = "purge ID: Force unload sound";

static const char *cmd_next_arg(const char *str)
{
	const unsigned char *ptr = (const unsigned char*)str;
	while (*ptr && !isspace(*ptr))
		++ptr;
	while (*ptr && isspace(*ptr))
		++ptr;
	return *ptr ? (const char*)ptr : NULL;
}

static void console_dump_info(struct console *c)
{
	char text[256];
	const char *scr_mode = "800x600";
	const char *music = "yes", *sound = "yes", *video = "yes";

	if (genie_mode & GENIE_MODE_1024_768)
		scr_mode = "1024x768";
	else if (genie_mode & GENIE_MODE_800_600)
		scr_mode = "800x600";
	else if (genie_mode & GENIE_MODE_640_480)
		scr_mode = "640x480";

	if (genie_mode & GENIE_MODE_NOMUSIC)
		music = "no";
	if (genie_mode & GENIE_MODE_NOSOUND)
		sound = "no";
	if (genie_mode & GENIE_MODE_NOVIDEO)
		video = "no";

	snprintf(text, sizeof text,
		"Screen resolution: %s\n"
		"Music playback: %s\n"
		"Sound playback: %s\n"
		"Video playback: %s",
		scr_mode, music, sound, video
	);
	console_puts(c, text);
}

static void console_toggle_border(struct genie_ui *ui)
{
	Uint32 flags = SDL_GetWindowFlags(ui->win);
	if (flags & SDL_WINDOW_BORDERLESS)
		SDL_SetWindowBordered(ui->win, SDL_TRUE);
	else
		SDL_SetWindowBordered(ui->win, SDL_FALSE);
}

static void console_run(struct console *c, char *str)
{
	char text[1024];
	/* ignore leading and trailing whitespace */
	unsigned char *ptr = (unsigned char*)str;
	while (*ptr && isspace(*ptr))
		++ptr;
	str = (char*)ptr;
	ptr = (unsigned char*)str + strlen(str);
	while (ptr > (unsigned char*)str && isspace(ptr[-1]))
		--ptr;
	*ptr = '\0';

	/* print the trimmed line that has been entered */
	snprintf(text, sizeof text, "%s%s", CONSOLE_PS1, str);
	console_puts(c, text);

	if (!*str)
		return;
	if (!strcmp(str, "quit"))
		exit(0);
#ifdef DEBUG
	/* Developer-only undocumented commands */
	else if (!strcmp(str, "hcf")) /* halt and catch fire */
		raise(SIGSEGV);
#endif
	else if (strsta(str, "help")) {
		const char *arg = cmd_next_arg(str);
		if (!arg) {
			if (strcmp(str, "help"))
				goto unknown;
			console_puts(c, help_console);
		} else if (!strcmp(arg, "quit"))
			console_puts(c, help_quit);
		else if (!strcmp(arg, "info"))
			console_puts(c, help_info);
		else if (!strcmp(arg, "help"))
			console_puts(c, help_console);
		else if (!strcmp(arg, "toggle"))
			console_puts(c, help_toggle);
		else {
			snprintf(text, sizeof text, "No help for \"%s\"", arg);
			console_puts(c, text);
		}
	} else if (!strcmp(str, "info"))
		console_dump_info(c);
	else if (strsta(str, "toggle")) {
		const char *arg = cmd_next_arg(str);
		if (!arg)
			console_puts(c, help_toggle);
		else if (!strcmp(arg, "border"))
			console_toggle_border(&genie_ui);
		else {
			snprintf(text, sizeof text, "Invalid option \"%s\"", arg);
			console_puts(c, text);
		}
	} else if (strsta(str, "play")) {
		const char *arg = cmd_next_arg(str);
		uint64_t num, dummy;
		if (!arg || parse_address(arg, &num, &dummy)) {
			console_puts(c, help_play);
			return;
		}
		if (num < 65536)
			genie_sfx_play(num);
	} else if (strsta(str, "purge")) {
		const char *arg = cmd_next_arg(str);
		uint64_t num, dummy;
		if (!arg || parse_address(arg, &num, &dummy)) {
			console_puts(c, help_purge);
			return;
		}
		if (num < 65536)
			genie_sfx_purge(num);
	} else {
unknown:
		snprintf(text, sizeof text, "Unknown command: \"%s\"\nType `help' for help", str);
		console_puts(c, text);
	}
}

static void console_key_down(struct genie_ui *ui, SDL_Event *ev)
{
	struct console *c = &ui->console;
	switch (ev->key.keysym.sym) {
	case '`':
		ui->console_show = 0;
		break;
	case '\r':
	case '\n':
		console_run(c, c->line_buffer);
		console_clear_line_buffer(c);
		break;
	case '\b':
		console_line_pop_last(c);
		break;
	}
	int ch = ev->key.keysym.sym;
	if (ch > 0xff)
		return;
	ch = ch & 0xff;
	if (ch >= ' ' && ch != '`' && ch != '\t' && isprint(ch))
		console_line_push_last(c, ch);
}

static void menu_key_down(struct genie_ui *ui, SDL_Event *ev)
{
	struct menu_nav *nav = genie_ui_menu_peek(ui);
	if (!nav)
		return;
	if (ui->console_show) {
		console_key_down(ui, ev);
		return;
	}

	switch (ev->key.keysym.sym) {
	case SDLK_DOWN:
		menu_nav_down(nav, MENU_KEY_DOWN);
		break;
	case SDLK_UP:
		menu_nav_down(nav, MENU_KEY_UP);
		break;
	case ' ':
		menu_nav_down(nav, MENU_KEY_SELECT);
		ui->menu_press = 1;
		genie_sfx_play(SFX_MENU_BUTTON);
		break;
	case '`':
		ui->console_show = 1;
		ui->menu_press = 0;
		break;
	}
}

static void menu_key_up(struct genie_ui *ui, SDL_Event *ev)
{
	struct menu_nav *nav = genie_ui_menu_peek(ui);
	if (!nav)
		return;
	if (ui->console_show)
		return;

	switch (ev->key.keysym.sym) {
	case ' ':
		menu_nav_up(nav, MENU_KEY_SELECT);
		ui->menu_press = 0;
		break;
	}
}

void genie_ui_key_down(struct genie_ui *ui, SDL_Event *ev)
{
	menu_key_down(ui, ev);
}

void genie_ui_key_up(struct genie_ui *ui, SDL_Event *ev)
{
	menu_key_up(ui, ev);
}

static int mouse_in_rect(int mx, int my, int x, int y, int w, int h)
{
	return mx >= x && mx < x + w && my >= y && my < y + h;
}

void genie_ui_mouse_down(struct genie_ui *ui, SDL_Event *ev)
{
	struct menu_nav *nav = genie_ui_menu_peek(ui);
	if (!nav || ui->console_show)
		return;

	const struct menu_list *list = nav->list;
	int mx = ev->button.x, my = ev->button.y;
	for (unsigned i = 0, n = list->count; i < n; ++i) {
		const struct genie_ui_button *btn = &list->buttons[i];
		if (mouse_in_rect(mx, my, btn->x, btn->y, btn->w, btn->h)) {
			nav->index = i;
			menu_nav_down(nav, MENU_KEY_SELECT);
			ui->menu_press = 1;
			genie_sfx_play(SFX_MENU_BUTTON);
			return;
		}
	}
}

void genie_ui_mouse_up(struct genie_ui *ui, SDL_Event *ev)
{
	struct menu_nav *nav = genie_ui_menu_peek(ui);
	if (!nav || ui->console_show)
		return;

	const struct menu_list *list = nav->list;
	int mx = ev->button.x, my = ev->button.y;
	for (unsigned i = 0, n = list->count; i < n; ++i) {
		const struct genie_ui_button *btn = &list->buttons[i];
		if (mouse_in_rect(mx, my, btn->x, btn->y, btn->w, btn->h)) {
			ui->menu_press = 0;
			if (i == nav->index)
				menu_nav_up(nav, MENU_KEY_SELECT);
			return;
		}
	}
}

void genie_ui_display(struct genie_ui *ui)
{
	genie_gfx_clear_screen(0, 0, 0, 0);
	genie_gfx_setup_ortho(ui->width, ui->height);

	if (ui->console_show)
		draw_console(ui);
	else {
		const struct menu_nav *nav = genie_ui_menu_peek(ui);
		if (nav)
			nav->display(ui);
		draw_menu(ui);
	}

	genie_ui_update(ui);
}

void genie_ui_menu_update(struct genie_ui *ui)
{
	const struct menu_nav *nav;
	const struct menu_list *list;

	nav = genie_ui_menu_peek(ui);
	list = nav->list;
	(void)list;
}

void genie_ui_menu_push(struct genie_ui *ui, struct menu_nav *nav)
{
	ui->stack[ui->stack_index++] = nav;

	genie_ui_menu_update(ui);
}

void genie_ui_menu_pop(struct genie_ui *ui)
{
	if (!--ui->stack_index) {
		ui->game->running = 0;
		return;
	}

	genie_ui_menu_update(ui);
}

void genie_ui_menu_pop2(struct genie_ui *ui, unsigned level)
{
	if (level > ui->stack_index) {
		show_error("Fatal error", "Menu navigation broken");
		exit(1);
		return;
	}
	ui->stack_index = level;
	genie_ui_menu_update(ui);
}

struct menu_nav *genie_ui_menu_peek(const struct genie_ui *ui)
{
	unsigned i;

	i = ui->stack_index;

	return i ? ui->stack[i - 1] : NULL;
}

void genie_ui_hide(struct genie_ui *ui)
{
	SDL_HideWindow(ui->win);
}

void genie_ui_show(struct genie_ui *ui)
{
	SDL_ShowWindow(ui->win);
	genie_ui_raise(ui);
}

void genie_ui_raise(struct genie_ui *ui)
{
	SDL_RaiseWindow(ui->win);
}

void genie_display_default(struct genie_ui *ui)
{
	draw_box(1, 0, ui->width - 1, ui->height);
}

void genie_display_game(struct genie_ui *ui)
{
	char buf[40];
	glColor3f(1, 1, 1);
	const struct genie_resources *res = genie_player_get_resources(&genie_game, 0);
	/* resources */
	snprintf(buf, sizeof buf, "%d", res->wood);
	genie_gfx_draw_text(32, 4, buf);
	snprintf(buf, sizeof buf, "%d", res->food);
	genie_gfx_draw_text(98, 4, buf);
	snprintf(buf, sizeof buf, "%d", res->gold);
	genie_gfx_draw_text(165, 4, buf);
	snprintf(buf, sizeof buf, "%d", res->stone);
	genie_gfx_draw_text(232, 4, buf);
	/* age */
	genie_gfx_draw_text(370, 4, civ_ages[0]);
}

void genie_display_single_player(struct genie_ui *ui)
{
	genie_display_default(ui);
	glColor3f(1, 1, 1);
	/* player configuration headers */
	genie_gfx_draw_text(37, 74, "Name");
	genie_gfx_draw_text(240, 74, "Civ");
	genie_gfx_draw_text(362, 74, "Player");
	genie_gfx_draw_text(466, 74, "Team");
	/* players */
	genie_gfx_draw_text(38, 110, "You");
	genie_gfx_draw_text(212, 110, "Yamato");
	glColor3f(0, 0, 1);
	genie_gfx_draw_text(402, 110, "1");
	glColor3f(1, 1, 1);
	genie_gfx_draw_text(490, 110, "-");
	genie_gfx_draw_text(38, 140, "Computer");
	genie_gfx_draw_text(212, 140, "Choson");
	genie_gfx_draw_text(490, 140, "-");
	/* player count */
	genie_gfx_draw_text(38, 374, "Number of Players");
	genie_gfx_draw_text(44, 410, "2");
	/* map configuration */
	genie_gfx_draw_text(530, 110, "Scenario: Random Map");
	genie_gfx_draw_text(530, 136, "Map Size: Medium");
	genie_gfx_draw_text(530, 166, "Map Type: Highland");
	genie_gfx_draw_text(530, 196, "Players: 2 - 8");
	genie_gfx_draw_text(530, 226, "Victory: Standard");
	genie_gfx_draw_text(530, 256, "Age: Default");
	genie_gfx_draw_text(530, 286, "Resources: Default");
	genie_gfx_draw_text(530, 316, "Difficulty Level: Moderate");
	genie_gfx_draw_text(530, 346, "Fixed Positions: Yes");
	genie_gfx_draw_text(530, 376, "Reveal Map: No");
	genie_gfx_draw_text(530, 406, "Full Tech: No");
	genie_gfx_draw_text(530, 436, "Path Finding: High");
}

void genie_display_diplomacy(struct genie_ui *ui)
{
	genie_display_default(ui);
	glColor3f(1, 1, 1);
	genie_gfx_draw_text(98, 157, "Name");
	genie_gfx_draw_text(230, 157, "Civilization");
	genie_gfx_draw_text(230, 157, "Civilization");
	genie_gfx_draw_text(312, 157, "Ally");
	genie_gfx_draw_text(383, 157, "Neutral");
	genie_gfx_draw_text(453, 157, "Enemy");
	genie_gfx_draw_text(98, 187, "You");
	genie_gfx_draw_text(98, 217, "Enemy");
	genie_gfx_draw_text(546, 223, "You need a market to\n pay tribute.");
	genie_gfx_draw_text(411, 437, "Allied victory");
}

void genie_display_achievements(struct genie_ui *ui)
{
	genie_display_default(ui);
	/* draw borders */
	glBegin(GL_QUADS);
		/* draw religion and survival headers */
		glColor3ub(8, 8, 8);
		glVertex2f(266, 138);
		glVertex2f(638, 138);
		glVertex2f(638, 506);
		glVertex2f(266, 506);
		/* draw technology header */
		glColor3ub(49, 49, 49);
		glVertex2f(352, 113);
		glVertex2f(547, 113);
		glVertex2f(547, 138);
		glVertex2f(352, 138);
		glVertex2f(420, 138);
		glVertex2f(484, 138);
		glVertex2f(484, 507);
		glVertex2f(420, 507);
		/* draw economy header */
		glVertex2f(191, 162);
		glVertex2f(352, 162);
		glVertex2f(352, 507);
		glVertex2f(191, 507);
		/* draw wonder header */
		glVertex2f(552, 162);
		glVertex2f(713, 162);
		glVertex2f(713, 507);
		glVertex2f(552, 507);
		/* draw military header */
		glColor3ub(8, 8, 8);
		glVertex2f(124, 187);
		glVertex2f(288, 187);
		glVertex2f(288, 507);
		glVertex2f(124, 507);
		/* draw total score header */
		glVertex2f(617, 187);
		glVertex2f(781, 187);
		glVertex2f(781, 507);
		glVertex2f(617, 507);
	glEnd();
	/* headers */
	glColor3f(1, 1, 1);
	genie_gfx_draw_text(128, 191, "Military");
	genie_gfx_draw_text(195, 166, "Economy");
	genie_gfx_draw_text(270, 141, "Religion");
	genie_gfx_draw_text(412, 116, "Technology");
	genie_gfx_draw_text(582, 141, "Survival");
	genie_gfx_draw_text(659, 166, "Wonder");
	genie_gfx_draw_text(704, 191, "Total");
	/* players */
	glColor3f(0, 0, 1);
	genie_gfx_draw_text(31, 225, "You");
	glColor3f(1, 0, 0);
	genie_gfx_draw_text(31, 263, "Enemy");
}

void genie_display_timeline(struct genie_ui *ui)
{
	genie_display_default(ui);
	/* world population graph */
	glBegin(GL_QUADS);
		glColor3f(0, 0, 1);
		glVertex2f(39, 153);
		glVertex2f(764, 153);
		glVertex2f(764, 312);
		glVertex2f(39, 312);
		glColor3f(1, 0, 0);
		glVertex2f(39, 312);
		glVertex2f(764, 312);
		glVertex2f(764, 472);
		glVertex2f(39, 472);
	glEnd();
	/* world population text */
	glColor3f(1, 1, 1);
	genie_gfx_draw_text(20, 115, "World population");
	genie_gfx_draw_text(40, 133, "6");
	genie_gfx_draw_text(42, 228, "You");
	genie_gfx_draw_text(42, 387, "Enemy");
	genie_gfx_draw_text(28, 482, "0:00");
	genie_gfx_draw_text(364, 500, "Time (Hr:Min)");
}
