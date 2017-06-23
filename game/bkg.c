#include "bkg.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../genie/drs.c"

#define startline(str,match) (!strncmp(str,match,strlen(match)))

static inline void bkg_arg(char *buf, size_t n, const char *start, const char *end)
{
	while (!isspace(*start))
		++start;
	while (isspace(*start))
		++start;
	if (start > end) {
		fputs("corrupt bkg\n", stderr);
		exit(1);
	}
	size_t narg = (size_t)(end - start);
	if (narg > n - 1) narg = n - 1;
	memcpy(buf, start, narg);
	buf[narg] = '\0';
}

static inline int bkg_map_col(const char *text, unsigned char col[2][3], unsigned index)
{
	unsigned char t[3];
	if (sscanf(text, "%hhu %hhu %hhu", &t[0], &t[1], &t[2]) != 3)
		return 1;
	col[index][0] = t[0];
	col[index][1] = t[1];
	col[index][2] = t[2];
	return 0;
}

int bkg_init(struct bkg *bkg, const void *data, size_t n)
{
	char buf[80];
	for (const char *next, *text = data, *end = (char*)data + n; text < end;) {
		if (!(next = strstr(text, "\r\n")))
			break;
		if (text == next) {
			text = next + 2;
			continue;
		}
		if startline(text, "background1_files") {
			bkg_arg(buf, sizeof buf, text, next);
			if (drs_fref_read(buf, &bkg->tbl[0])) {
				fprintf(stderr, "bad bkg1: %s\n", buf);
				goto fail;
			}
		} else if startline(text, "background2_files") {
			bkg_arg(buf, sizeof buf, text, next);
			if (drs_fref_read(buf, &bkg->tbl[1])) {
				fprintf(stderr, "bad bkg2: %s\n", buf);
				goto fail;
			}
		} else if startline(text, "background3_files") {
			bkg_arg(buf, sizeof buf, text, next);
			if (drs_fref_read(buf, &bkg->tbl[2])) {
				fprintf(stderr, "bad bkg3: %s\n", buf);
				goto fail;
			}
		} else if startline(text, "palette_file") {
			bkg_arg(buf, sizeof buf, text, next);
			if (drs_ref_read(buf, &bkg->palette)) {
				fprintf(stderr, "bad palette: %s\n", buf);
				goto fail;
			}
		} else if startline(text, "cursor_file") {
			bkg_arg(buf, sizeof buf, text, next);
			if (drs_ref_read(buf, &bkg->cursor)) {
				fprintf(stderr, "bad cursor: %s\n", buf);
				goto fail;
			}
		} else if startline(text, "shade_amount") {
			bkg_arg(buf, sizeof buf, text, next);
			if (drs_ref_read(buf, &bkg->shade)) {
				fprintf(stderr, "bad shade: %s\n", buf);
				goto fail;
			}
		} else if startline(text, "button_file") {
			bkg_arg(buf, sizeof buf, text, next);
			if (drs_ref_read(buf, &bkg->button)) {
				fprintf(stderr, "bad file: %s\n", buf);
				goto fail;
			}
		} else if startline(text, "popup_dialog_sin") {
			bkg_arg(buf, sizeof buf, text, next);
			if (drs_ref_read(buf, &bkg->popup)) {
				fprintf(stderr, "bad popup: %s\n", buf);
				goto fail;
			}
		} else if startline(text, "background_pos") {
			bkg_arg(buf, sizeof buf, text, next);
			if (sscanf(buf, "%u", &bkg->position) != 1) {
				fprintf(stderr, "bad position: %s\n", buf);
				goto fail;
			}
		} else if startline(text, "background_color") {
			bkg_arg(buf, sizeof buf, text, next);
			if (sscanf(buf, "%u", &bkg->col.index) != 1) {
				fprintf(stderr, "bad color index: %s\n", buf);
				goto fail;
			}
		} else if startline(text, "bevel_colors") {
			unsigned char b[6];
			bkg_arg(buf, sizeof buf, text, next);
			if (sscanf(buf, "%hhu %hhu %hhu %hhu %hhu %hhu", &b[0], &b[1], &b[2], &b[3], &b[4], &b[5]) != 6) {
				fprintf(stderr, "bad bevel colors: %s\n", buf);
				goto fail;
			}
			bkg->col.bevel[0] = b[0];
			bkg->col.bevel[1] = b[1];
			bkg->col.bevel[2] = b[2];
			bkg->col.bevel[3] = b[3];
			bkg->col.bevel[4] = b[4];
			bkg->col.bevel[5] = b[5];
		} else if startline(text, "text_color1") {
			bkg_arg(buf, sizeof buf, text, next);
			if (bkg_map_col(buf, bkg->col.text, 0)) {
				fprintf(stderr, "bad text color1: %s\n", buf);
				goto fail;
			}
		} else if startline(text, "text_color2") {
			bkg_arg(buf, sizeof buf, text, next);
			if (bkg_map_col(buf, bkg->col.text, 1)) {
				fprintf(stderr, "bad text color2: %s\n", buf);
				goto fail;
			}
		} else if startline(text, "focus_color1") {
			bkg_arg(buf, sizeof buf, text, next);
			if (bkg_map_col(buf, bkg->col.focus, 0)) {
				fprintf(stderr, "bad focus color1: %s\n", buf);
				goto fail;
			}
		} else if startline(text, "focus_color2") {
			bkg_arg(buf, sizeof buf, text, next);
			if (bkg_map_col(buf, bkg->col.focus, 1)) {
				fprintf(stderr, "bad focus color2: %s\n", buf);
				goto fail;
			}
		} else if startline(text, "state_color1") {
			bkg_arg(buf, sizeof buf, text, next);
			if (bkg_map_col(buf, bkg->col.state, 0)) {
				fprintf(stderr, "bad state color1: %s\n", buf);
				goto fail;
			}
		} else if startline(text, "state_color2") {
			bkg_arg(buf, sizeof buf, text, next);
			if (bkg_map_col(buf, bkg->col.state, 1)) {
				fprintf(stderr, "bad state color2: %s\n", buf);
				goto fail;
			}
		} else {
			char buf[80];
			size_t bufn = (size_t)(next - text);
			if (bufn > (sizeof buf) - 1)
				bufn = (sizeof buf) - 1;
			memcpy(buf, text, bufn);
			buf[bufn] = '\0';
			fprintf(stderr, "bkg: unknown property: %s\n", buf);
			text = next + 2;
			continue;
		}
		text = next + 2;
	}
	return 0;
fail:
	return 1;
}
