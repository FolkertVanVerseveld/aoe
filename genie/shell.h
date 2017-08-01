/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

/**
 * Genie game engine integrated shell
 *
 * Licensed under Affero General Public License v3.0
 * Copyright by Folkert van Verseveld.
 *
 * Every serious open source probably has a shell for unleashing the powerfull application and its features.
 * Our shell is pretty basic, but we have to start somewhere ;-)
 */

#ifndef GENIE_SHELL_H
#define GENIE_SHELL_H

#define GENIE_CONSOLE_ROWS 40
#define GENIE_CONSOLE_LINE_MAX 80

#define CONSOLE_PS1 "# "

struct console {
	struct {
		char text[GENIE_CONSOLE_ROWS][GENIE_CONSOLE_LINE_MAX];
		unsigned start, index, size;
	} screen;
	char line_buffer[GENIE_CONSOLE_LINE_MAX];
	unsigned line_count;
};

void console_init(struct console *c);
void console_puts(struct console *c, const char *str);
void console_line_pop_last(struct console *c);
void console_line_push_last(struct console *c, int ch);

#endif
