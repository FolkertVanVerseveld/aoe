/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <png.h>

int main(void)
{
	int retval = 1;
	FILE *f = fopen("a.png", "wb");
	png_structp png_ptr = NULL;
	png_infop info_ptr = NULL;
	png_bytep row = NULL;
	int width = 320, height = 200;
	const char *title = "Mah boi";

	srand(time(NULL));

	if (!f) {
		perror("a.png");
		goto fail;
	}
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr) {
		fputs("png: no write struct\n", stderr);
		goto fail;
	}
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		fputs("png: no info struct\n", stderr);
		goto fail;
	}
	if (setjmp(png_jmpbuf(png_ptr))) {
		fputs("png: write error\n", stderr);
		goto fail;
	}
	png_init_io(png_ptr, f);
	/* write header */
	png_set_IHDR(
		png_ptr, info_ptr,
		width, height, 8, PNG_COLOR_TYPE_RGBA,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE,
		PNG_FILTER_TYPE_BASE
	);
	if (title) {
		png_text title_text;
		title_text.compression = PNG_TEXT_COMPRESSION_NONE;
		title_text.key = "Title";
		title_text.text = (char*)title;
		png_set_text(png_ptr, info_ptr, &title_text, 1);
	}
	png_write_info(png_ptr, info_ptr);
	row = malloc(4 * width * sizeof(png_byte));
	/* write image data */
	int x, y;
	for (y = 0; y < height; ++y) {
		for (x = 0; x < width; ++x) {
			row[4 * x + 0] = rand();
			row[4 * x + 1] = rand();
			row[4 * x + 2] = rand();
			row[4 * x + 3] = rand();
		}
		png_write_row(png_ptr, row);
	}
	png_write_end(png_ptr, NULL);
	retval = 0;
fail:
	if (f)
		fclose(f);
	if (info_ptr)
		png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
	if (png_ptr)
		png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
	if (row)
		free(row);
	return retval;
}
