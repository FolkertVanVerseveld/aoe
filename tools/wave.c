/* licensed under Affero General Public License version 3 */
/*
Simple wave inspector for data resource sets.
*/
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <AL/al.h>
#include <AL/alc.h>
#include "../genie/drs.h"

#define RIFF_MAGIC 0x46464952
#define WAVE_MAGIC 0x45564157
#define FORMAT_MAGIC 0x20746d66
#define DATA_MAGIC 0x61746164

struct ckfmt {
	uint16_t format;
	uint16_t channels;
	uint32_t freq;
	uint32_t bytes;
	uint16_t align;
	uint16_t sample;
};

static ALuint src = AL_NONE, buf = AL_NONE;

static inline int alchk(void)
{
	ALenum al_err;
	if ((al_err = alGetError()) != AL_NO_ERROR)
		fprintf(stderr, "OpenAL error: %x\n", al_err);
	return al_err != AL_NO_ERROR;
}

static int sfx_play(char *data, struct ckfmt *fmt, size_t cksz)
{
	printf("%hu,%hu,%u,", fmt->format, fmt->channels, fmt->freq);
	printf("%u,%hu,%hu\n", fmt->bytes, fmt->align, fmt->sample);
	switch (fmt->format) {
		case 1:
			printf("PCM%u\n", fmt->sample);
			break;
		case 7:
			puts("uLaw");
			fputs("not supported yet\n", stderr);
			return 0;
		default:
			fprintf(stderr, "unknown format %u\n", fmt->format);
			return 1;
	}
	ALenum format;
	int ret = 1;
	switch (fmt->channels) {
	case 1:
		switch (fmt->sample) {
		case  8: format = AL_FORMAT_MONO8;  break;
		case 16: format = AL_FORMAT_MONO16; break;
		default:
			fprintf(stderr, "bad sample size: %u\n", fmt->sample);
			goto fail;
		}
	case 2:
		switch (fmt->sample) {
		case  8: format = AL_FORMAT_STEREO8;  break;
		case 16: format = AL_FORMAT_STEREO16; break;
		default:
			fprintf(stderr, "bad sample size: %u\n", fmt->sample);
			goto fail;
		}
	}
	alBufferData(buf, format, data, cksz, fmt->freq / 2);
	ALint wave_size;
	alGetBufferi(buf, AL_SIZE, &wave_size);
	if (alchk())
		fputs("queuing error\n", stderr);
	printf("wave size: %d bytes\n", wave_size);
	alSourcei(src, AL_BUFFER, buf);
	alSourcei(src, AL_LOOPING, AL_TRUE);
	alSourcef(src, AL_GAIN, 1);
	alSourcePlay(src);
	if (alchk())
		fputs("playback failed\n", stderr);
	else {
		while (getchar() != '\n') ;
		alSourceStop(src);
	}
fail:
	alSourcei(src, AL_BUFFER, AL_NONE);
	alchk();
	return ret;
}

int drsmap_stat(char *data, size_t size)
{
	if (size < sizeof(struct drsmap)) {
		fputs("invalid drs file\n", stderr);
		return 1;
	}
	struct drsmap *drs = (struct drsmap*)data;
	if (strncmp("1.00tribe", drs->version, strlen("1.00tribe"))) {
		fputs("invalid drs file\n", stderr);
		return 1;
	}
	if (!drs->nlist)
		return 0;
	struct drs_list *list = (struct drs_list*)((char*)drs + sizeof(struct drsmap));
	for (unsigned i = 0; i < drs->nlist; ++i, ++list) {
		if (list->type != DT_WAVE)
			continue;
		// resource offset or something
		if (list->offset > size) {
			fputs("bad item offset\n", stderr);
			return 1;
		}
		struct drs_item *item = (struct drs_item*)(data + list->offset);
		for (unsigned j = 0; j < list->size; ++j, ++item) {
			if (list->offset + (j + 1) * sizeof(struct drs_item) > size) {
				fputs("bad list\n", stderr);
				return 1;
			}
			uint32_t *magic = (uint32_t*)(data + item->offset);
			if (*magic != RIFF_MAGIC) {
				fprintf(stderr, "bad riff at %x: magic=%x\n", item->offset, *magic);
				return 1;
			}
			if (magic[2] != WAVE_MAGIC) {
				fprintf(stderr, "bad wave at %x: magic=%x\n", item->offset + 4, magic[2]);
				return 1;
			}
			struct ckfmt *fmt = NULL;
			char *ckdata = NULL;
			size_t datasz = 0;
			for (uint32_t c_off = 12; c_off < magic[1];) {
				uint32_t *c_data = (uint32_t*)(data + item->offset + c_off);
				uint32_t cksz = c_data[1];
				if (!cksz) {
					fprintf(stderr, "empty chunk at %x\n", item->offset + c_off);
					return 1;
				}
				if (c_data[0] == FORMAT_MAGIC) {
					if (cksz < 16) {
						fprintf(stderr, "bad format chunk at %x: size=%x\n", item->offset + c_off, cksz);
						return 1;
					}
					if (fmt)
						fprintf(stderr, "overriding format at %x\n", item->offset + c_off);
					fmt = (struct ckfmt*)(data + item->offset + c_off + 8);
				} else if (c_data[0] == DATA_MAGIC) {
					if (ckdata)
						fprintf(stderr, "overriding data at %x\n", item->offset + c_off);
					ckdata = data + item->offset + c_off;
					datasz = 8 * (cksz / 8);
				}
				c_off += cksz + 8;
			}
			if (!fmt) {
				fputs("no format\n", stderr);
				return 1;
			}
			if (!ckdata) {
				fputs("no data\n", stderr);
				return 1;
			}
			printf("id: %u\n", item->id);
			sfx_play(ckdata + 8, fmt, datasz);
		}
	}
	return 0;
}

int main(int argc, char **argv)
{
	int ret = 1;
	FILE *f = NULL;
	char *drs_sfx = NULL;
	ALCdevice *dev = NULL;
	ALCcontext *ctx = NULL;
	struct stat st_sfx;
	if (argc != 2) {
		fprintf(stderr, "usage: %s path\n", argc > 0 ? argv[0] : "wave");
		goto fail;
	}
	const char *drs = argv[1];
	if (!(f = fopen(drs, "rb")) || stat(drs, &st_sfx)) {
		perror(drs);
		goto fail;
	}
	size_t n = st_sfx.st_size;
	printf("%s: %zu bytes\n", drs, n);
	if (!(drs_sfx = malloc(n)) || fread(drs_sfx, 1, n, f) != n) {
		perror(drs);
		goto fail;
	}
	fclose(f);
	const char *dname = NULL;
	if (alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT"))
		dname = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
	dev = alcOpenDevice(dname);
	if (!dev) {
		fputs("alc device error\n", stderr);
		goto fail;
	}
	ctx = alcCreateContext(dev, NULL);
	if (!ctx) {
		fputs("alc context error\n", stderr);
		goto fail;
	}
	alcMakeContextCurrent(ctx);
	f = NULL;
	alGenSources(1, &src);
	alGenBuffers(1, &buf);
	if (alchk()) {
		fputs("buffer error\n", stderr);
		goto fail;
	}
	ret = drsmap_stat(drs_sfx, (size_t)st_sfx.st_size);
fail:
	if (buf != AL_NONE)
		alDeleteBuffers(1, &buf);
	if (src != AL_NONE)
		alDeleteSources(1, &src);
	if (ctx)
		alcDestroyContext(ctx);
	if (dev)
		alcCloseDevice(dev);
	if (drs_sfx)
		free(drs_sfx);
	if (f)
		fclose(f);
	return ret;
}
