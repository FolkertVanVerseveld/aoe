#include "shell.h"
#include "string.h"

#include <stdio.h>
#include <string.h>

void console_init(struct console *c)
{
	c->screen.start = c->screen.index = c->screen.size = 0;
}

static void console_add_line(struct console *c, const char *str)
{
	char text[GENIE_CONSOLE_LINE_MAX];
	strncpy0(text, str, GENIE_CONSOLE_LINE_MAX);
	memcpy(c->screen.text[c->screen.index], text, GENIE_CONSOLE_LINE_MAX);

	if (c->screen.size < GENIE_CONSOLE_ROWS)
		++c->screen.size;
	else
		c->screen.start = (c->screen.start + 1) % GENIE_CONSOLE_ROWS;
	c->screen.index = (c->screen.index + 1) % GENIE_CONSOLE_ROWS;
}

void console_puts(struct console *c, const char *str)
{
	while (1) {
		const char *end = strchr(str, '\n');
		if (!end) {
			puts(str);
			if (*str)
				console_add_line(c, str);
			break;
		}
		char text[GENIE_CONSOLE_LINE_MAX];
		size_t n = end - str + 1;
		text[0] = '\0';
		strncpy0(text, str, n);

		if (n >= sizeof text)
			--n;
		if (n)
			text[n] = '\0';
		puts(text);
		console_add_line(c, text);
		str = end + 1;
	}
}
