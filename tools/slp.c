/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <err.h>

#include <png.h>

struct palette {
	uint32_t table[256];
	unsigned count;
};

struct slp_frame_info {
	uint32_t ctbladdr;
	uint32_t otbladdr;
	uint32_t paladdr;
	uint32_t prop;
	uint32_t width;
	uint32_t height;
	int32_t hsx;
	int32_t hsy;
};

struct slp_row {
	uint16_t left;
	uint16_t right;
};

struct slp {
	char version_name[4];
	uint32_t num_frames;
	char comment[24];
};

/** copies from src to dest and sets (char*)dest + n = '\0' */
static void *null_memcpy(void *dest, const void *src, size_t n)
{
	void *ptr = memcpy(dest, src, n);
	*((char*)dest + n) = '\0';
	return ptr;
}

static char *dos_fgets(char *s, int size, FILE *stream)
{
	char *ptr = fgets(s, size, stream);
	if (size) {
		size_t n = strlen(ptr);
		if (n) {
			ptr[n - 1] = '\0';
			if (n - 1 && ptr[n - 2] == '\r')
				ptr[n - 2] = '\0';
		}
	}
	return ptr;
}

static int palette_read(struct palette *pal, const char *name)
{
	int retval = 1;
	char line[80];
	FILE *f;
	unsigned count;

	f = fopen(name, "rb");
	if (!f) {
		perror(name);
		goto fail;
	}
	if (!dos_fgets(line, sizeof line, f))
		goto eof;
	if (strcmp("JASC-PAL", line))
		goto bad;

	if (!dos_fgets(line, sizeof line, f))
		goto eof;
	if (strcmp("0100", line))
		goto bad;

	if (!dos_fgets(line, sizeof line, f))
		goto eof;
	if (sscanf(line, "%u", &count) != 1 || count < 1 || count > 256) {
bad:
		fputs("not a palette\n", stderr);
		goto fail;
	}
	for (unsigned i = 0; i < count; ++i) {
		unsigned char r, g, b, a = 0xff;
		if (!dos_fgets(line, sizeof line, f))
			goto fail;
		if (sscanf(line, "%hhu %hhu %hhu", &r, &g, &b) != 3) {
			fputs("corrupt palette\n", stderr);
			goto fail;
		}
		pal->table[i] = r << 24 | g << 16 | b << 8 | a;
		//printf("%02hhX: %02hhX %02hhX %02hhX %02hhX\n", i, r, g, b, a);
	}
eof:
	if (feof(f) && ferror(f)) {
		perror(name);
		goto fail;
	}
	pal->count = count;
	retval = 0;
fail:
	if (f)
		fclose(f);
	return retval;
}

static struct slp_row *get_row(const void *data, uint32_t pos);

static struct slp_frame_info *get_frame(const void *data, unsigned frame);
static void slp_frame_info_dump(const struct slp_frame_info *f, const void *data);
static uint32_t *get_cmd(const void *data, uint32_t pos);

#define CMD_LESS_BLK_CPY0 0x00
#define CMD_LESS_BLK_CPY1 0x04
#define CMD_LESS_BLK_CPY2 0x08
#define CMD_LESS_BLK_CPY3 0x0c

#define CMD_LESS_SKIP0 0x01
#define CMD_LESS_SKIP1 0x05
#define CMD_LESS_SKIP2 0x09
#define CMD_LESS_SKIP3 0x0D

#define CMD_GREAT_BLK_CPY 0x02

#define CMD_EOL 0x0F

static void row_set_pixel(png_bytep row, const struct palette *p, unsigned x, unsigned id)
{
	uint32_t rgba = p->table[id];
	row[4 * x + 0] = (rgba >> 24) & 0xff;
	row[4 * x + 1] = (rgba >> 16) & 0xff;
	row[4 * x + 2] = (rgba >> 8) & 0xff;
	row[4 * x + 3] = 0xff;
}

static void less_blk_cpy(png_bytep row, const struct palette *p, const uint8_t **cmd, unsigned *x)
{
	const uint8_t *ptr = *cmd;
	unsigned pos = *x;

	switch (*ptr & 0xf) {
	case CMD_LESS_BLK_CPY0:
		++ptr;
		*x = pos + 1;
		break;
	case CMD_LESS_BLK_CPY1:
		row_set_pixel(row, p, pos + 0, ptr[1]);
		ptr += 2;
		*x = pos + 1;
		break;
	case CMD_LESS_BLK_CPY2:
		row_set_pixel(row, p, pos + 0, ptr[1]);
		row_set_pixel(row, p, pos + 1, ptr[2]);
		ptr += 3;
		*x = pos + 2;
		break;
	case CMD_LESS_BLK_CPY3:
		row_set_pixel(row, p, pos + 0, ptr[1]);
		row_set_pixel(row, p, pos + 1, ptr[2]);
		row_set_pixel(row, p, pos + 2, ptr[3]);
		ptr += 4;
		*x = pos + 3;
		break;
	}

	*cmd = ptr;
}

static void great_blk_cpy(png_bytep row, const struct palette *p, const uint8_t **cmd, unsigned *x)
{
	const uint8_t *ptr = *cmd;
	unsigned pos = *x;

	unsigned sub = *ptr & 0xf0;

	unsigned n = (sub << 4) + ptr[1];

	printf("sub = %X, ptr[1] = %X, n = %u\n", sub, ptr[1], n);

	++ptr;
	*x = pos + 1;
	*cmd = ptr;
}

static void less_skip(png_bytep row, const struct palette *p, const uint8_t **cmd, unsigned *x)
{
	const uint8_t *ptr = *cmd;
	unsigned n, pos = *x;

	switch (*ptr & 0xf) {
	case CMD_LESS_SKIP0:
		/* pixel count is in next byte */
		for (n = ptr[1]; n; --n)
			row[4 * (pos + n) + 3] = 0;
		*x = pos + n + 1;
		break;
	case CMD_LESS_SKIP1:
		row[4 * pos + 3] = 0;
		*x = pos + 1;
		break;
	case CMD_LESS_SKIP2:
		row[4 * pos + 3] = 0;
		row[4 * pos + 7] = 0;
		*x = pos + 2;
		break;
	case CMD_LESS_SKIP3:
		row[4 * pos + 3] = 0;
		row[4 * pos + 7] = 0;
		row[4 * pos + 11] = 0;
		*x = pos + 3;
		break;
	}

	*cmd = ++ptr;
}

static int write_frame(png_structp png, const struct palette *pal, const struct slp_frame_info *f, const void *data, unsigned width, unsigned height)
{
	int retval = 1;
	png_bytep row = NULL;
	const struct slp_row *slp_row = get_row(data, f->otbladdr);

	puts("write frame...");

	row = malloc(4 * width * sizeof(png_byte));
	if (!row) {
		fputs("out of memory\n", stderr);
		goto fail;
	}
	unsigned y, x, xp;
	const uint32_t *cmdaddr = get_cmd(data, f->ctbladdr);
	for (y = 0; y < height; ++y, ++slp_row, ++cmdaddr) {
		printf("%5" PRId32 ": %5" PRIx32 "\n", y, *cmdaddr);
		for (x = 0; x < width; ++x) {
			row[4 * x + 0] = 0;
			row[4 * x + 1] = 0;
			row[4 * x + 2] = 0;
			row[4 * x + 3] = 0;
		}
		const uint8_t *cmd = (const uint8_t*)data + *cmdaddr;
		for (x = slp_row->left, xp = width - slp_row->right; x < xp;) {
			row[4 * x + 3] = 0xff; /* FIXME remove when cmd switch completed */
			#if 1
			switch (*cmd & 0xf) {
			case CMD_LESS_BLK_CPY0:
			case CMD_LESS_BLK_CPY1:
			case CMD_LESS_BLK_CPY2:
			case CMD_LESS_BLK_CPY3:
				printf("cpy %hhu\n", *cmd & 0xf);
				less_blk_cpy(row, pal, &cmd, &x);
				break;
			case CMD_LESS_SKIP0:
			case CMD_LESS_SKIP1:
			case CMD_LESS_SKIP2:
			case CMD_LESS_SKIP3:
				printf("skip %hhu\n", *cmd & 0xf);
				less_skip(row, pal, &cmd, &x);
				break;
			case CMD_GREAT_BLK_CPY:
				printf("big cpy %hhu\n", *cmd & 0xf);
				great_blk_cpy(row, pal, &cmd, &x);
				break;
			default:
				#if 1
				fprintf(stderr, "ignore cmd: %02hhX\n", *cmd & 0xf);
				//x = width - slp_row->right;
				++x;
				#else
				//errx(1, "bad cmd: %02hhX", *cmd & 0xf);
				#endif
				break;
			case CMD_EOL:
				++x;
				break;
			}
			#else
			++x;
			#endif
		}
		png_write_row(png, row);
	}
	retval = 0;
fail:
	if (row)
		free(row);
	return retval;
}

static int write_slp_png(unsigned f_id, const void *data, const struct palette *pal)
{
	int retval = 1;
	FILE *f = NULL;
	png_structp png = NULL;
	png_infop info = NULL;
	png_bytep row = NULL;
	char filename[24];
	snprintf(filename, sizeof filename, "%02u.png", f_id);
	const char *title = "slp";
	const struct slp_frame_info *frame = NULL;
	unsigned width, height;

	/* fetch dimensions */
	frame = get_frame(data, f_id);
	width = frame->width;
	height = frame->height;
	/* setup stream for writing png */
	f = fopen(filename, "wb");
	if (!f) {
		perror(filename);
		goto fail;
	}
	png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png) {
		fputs("png: no write struct\n", stderr);
		goto fail;
	}
	info = png_create_info_struct(png);
	if (!info) {
		fputs("png: no info struct\n", stderr);
		goto fail;
	}
	if (setjmp(png_jmpbuf(png))) {
		fputs("png: write error\n", stderr);
		goto fail;
	}
	png_init_io(png, f);
	/* write header */
	png_set_IHDR(
		png, info,
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
		png_set_text(png, info, &title_text, 1);
	}
	png_write_info(png, info);
	if (write_frame(png, pal, frame, data, width, height)) {
		fputs("slp: bad frame\n", stderr);
		goto fail;
	}
	png_write_end(png, NULL);
	retval = 0;
fail:
	if (f)
		fclose(f);
	if (info)
		png_free_data(png, info, PNG_FREE_ALL, -1);
	if (png)
		png_destroy_write_struct(&png, (png_infopp)NULL);
	if (row)
		free(row);
	return retval;
}

static struct slp_frame_info *get_frame(const void *data, unsigned frame)
{
	const struct slp_frame_info *table;
	table = (const struct slp_frame_info*)((char*)data + sizeof(struct slp));
	return (struct slp_frame_info*)&table[frame];
}

static struct slp_row *get_row(const void *data, uint32_t pos)
{
	const struct slp_row *row;
	row = (const struct slp_row*)((char*)data + pos);
	return (struct slp_row*)row;
}

static uint32_t *get_cmd(const void *data, uint32_t pos)
{
	return (uint32_t*)((char*)data + pos);
}

static void slp_frame_info_dump(const struct slp_frame_info *f, const void *data)
{
	printf("command table offset: %" PRIx32 "\n", f->ctbladdr);
	printf("outline table offset: %" PRIx32 "\n", f->otbladdr);
	printf("palette offset: %" PRIx32 "\n", f->paladdr);
	printf("properties: %" PRIx32 "\n", f->prop);
	printf("width: %" PRIu32 "\n", f->width);
	printf("height: %" PRIu32 "\n", f->height);
	printf("hotspot x: %" PRId32 "\n", f->hsx);
	printf("hotspot y: %" PRId32 "\n", f->hsy);

	const struct slp_row *row = get_row(data, f->otbladdr);
	for (uint32_t i = 0, n = f->height; i < n; ++i, ++row) {
		printf("%5" PRId32 ": %5" PRIu16 ",%5" PRIu16 "\n", i, row->left, row->right);
	}

#if 0
	const uint32_t *cmd = get_cmd(data, f->ctbladdr);
	for (uint32_t i = 0; i < f->height; ++i, ++cmd)
		printf("%5" PRId32 ": %5" PRIx32 "\n", i, *cmd);
#endif
}

static int slp_dump(const void *data, size_t size)
{
	const struct slp *slp = data;
	char buf[80];
	size_t offset = 0;

	if (size < sizeof *slp)
		return -1;

	null_memcpy(buf, slp->version_name, sizeof slp->version_name);
	printf("version: %s\n", buf);
	null_memcpy(buf, slp->comment, sizeof slp->comment);
	printf("comment: %s\n", buf);
	printf("number of frames: %" PRIu32 "\n", slp->num_frames);

	offset += sizeof *slp;
	const struct slp_frame_info *frame;
	size_t frame_size = slp->num_frames * sizeof *frame;
	if (size - offset < frame_size)
		return -1;
	for (uint32_t i = 0, n = slp->num_frames; i < n; ++i) {
		printf("frame %" PRId32 "\n", i);
		slp_frame_info_dump(get_frame(data, i), data);
	}

	return 0;
}

int main(int argc, char **argv)
{
	FILE *f = NULL;
	int retval = 1;
	struct stat st;
	void *data = NULL;
	struct palette pal;

	if (argc != 3) {
		fprintf(stderr, "usage: %s file palette\n", argc > 0 ? argv[0] : "slp");
		goto fail;
	}
	retval = palette_read(&pal, argv[2]);
	if (retval)
		goto fail;
	f = fopen(argv[1], "rb");
	if (!f || stat(argv[1], &st)) {
		perror(argv[1]);
		goto fail;
	}
	data = malloc(st.st_size);
	if (!data || fread(data, st.st_size, 1, f) != 1) {
		perror(argv[1]);
		goto fail;
	}
	retval = slp_dump(data, st.st_size);
	if (retval) {
		fputs("bad slp file\n", stderr);
		goto fail;
	}
	retval = write_slp_png(0, data, &pal);
	if (retval) {
		fputs("could not extract image\n", stderr);
		goto fail;
	}

	retval = 0;
fail:
	if (data)
		free(data);
	if (f)
		fclose(f);
	return retval;
}
