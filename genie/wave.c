/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "wave.h"

#include <stdio.h>
#include <string.h>

#define RIFF_MAGIC 0x46464952
#define WAVE_MAGIC 0x45564157
#define FORMAT_MAGIC 0x20746d66
#define DATA_MAGIC 0x61746164

static int parse_riff(const char *data, size_t filesz, char **sdata, struct ckfmt *sfmt, size_t *scksz, char *err, size_t errlen)
{
	uint32_t *magic = (uint32_t*)data;
	if (*magic != RIFF_MAGIC) {
		snprintf(err, errlen, "bad riff: magic=%x", *magic);
		return 1;
	}
	if (magic[2] != WAVE_MAGIC) {
		snprintf(err, errlen, "bad wave: magic=%x", magic[2]);
		return 1;
	}
	const struct ckfmt *fmt = NULL;
	const char *ckdata = NULL;
	size_t datasz = 0;
	for (uint32_t c_off = 12; c_off < magic[1];) {
		uint32_t *c_data = (uint32_t*)(data + c_off);
		uint32_t cksz = c_data[1];
		if (!cksz) {
			snprintf(err, errlen, "empty chunk at %x", c_off);
			return 1;
		}
		if (c_data[0] == FORMAT_MAGIC) {
			if (cksz < 16) {
				snprintf(err, errlen, "bad format chunk at %x: size=%x", c_off, cksz);
				return 1;
			}
			if (c_off + 8 + sizeof(struct ckfmt) > filesz)
				break;
			if (fmt)
				snprintf(err, errlen, "overriding format at %x", c_off);
			fmt = (struct ckfmt*)(data + c_off + 8);
		} else if (c_data[0] == DATA_MAGIC) {
			if (ckdata)
				snprintf(err, errlen, "overriding data at %x", c_off);
			ckdata = data + c_off;
			datasz = 8 * (cksz / 8);
		}
		c_off += cksz + 8;
		if (c_off + sizeof(uint32_t) >= filesz)
			break;
	}
	if (!fmt) {
		strcpy(err, "no format");
		return 1;
	}
	if (!ckdata) {
		strcpy(err, "no data");
		return 1;
	}
	*sdata = (char*)ckdata + 8;
	*sfmt = *fmt;
	*scksz = datasz;
	return 0;
}

int wave_load(struct wave *w, const void *data, size_t size)
{
	char err[256];
	int ret = 1;
	char *map, *sfx;
	size_t cksz;
	struct ckfmt fmt;
	map = (void*)data;
	if (parse_riff(map, size, &sfx, &fmt, &cksz, err, sizeof err))
		goto fail;
	printf("%hu,%hu,%hu,%u,%hu,%hu\n",
		fmt.format, fmt.channels, fmt.freq,
		fmt.bytes, fmt.align, fmt.sample
	);
	switch (fmt.format) {
	case 1:
		printf("PCM%u\n", fmt.sample);
		break;
	case 7:
		puts("uLaw");
		fputs("not supported yet\n", stderr);
		return 1;
	default:
		fprintf(stderr, "unknown format %u\n", fmt.format);
		return 1;
	}
	ALenum format;
	switch (fmt.channels) {
	case 1:
		switch (fmt.sample) {
		case  8: format = AL_FORMAT_MONO8;  break;
		case 16: format = AL_FORMAT_MONO16; break;
		default:
			fprintf(stderr, "bad sample size: %u\n", fmt.sample);
			goto fail;
		}
	case 2:
		switch (fmt.sample) {
		case  8: format = AL_FORMAT_STEREO8;  break;
		case 16: format = AL_FORMAT_STEREO16; break;
		default:
			fprintf(stderr, "bad sample size: %u\n", fmt.sample);
			goto fail;
		}
	}
	w->data = sfx;
	w->size = size;
	w->cksz = cksz;
	w->fmt = fmt;
	alGenBuffers(1, &w->buf);
	printf("w->buf: %d\n", w->buf);
	alBufferData(w->buf, format, w->data, cksz, fmt.freq / 2);
	ret = 0;
fail:
	return ret;
}
