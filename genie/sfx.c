/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "sfx.h"

#include <stdio.h>
#include <stdlib.h>
#include <err.h>

#include <AL/al.h>
#include <AL/alc.h>
#include <SDL2/SDL_mixer.h>

#include "dbg.h"
#include "dmap.h"
#include "engine.h"
#include "prompt.h"
#include "wave.h"

#define SFX_INIT 1
#define SFX_INIT_DEV 2
#define SFX_INIT_HEAP 4
#define SFX_INIT_SRC 8

static int sfx_init = 0;

#define MUSIC_CHANNEL 0
/* We need more than one for fade-in/fade-out effects */
#define MUSIC_CHANNEL_COUNT 2
/* Should be sufficient for now */
#define SFX_CHANNEL_COUNT 20

#define SFX_HEAP_DEFAULT_SIZE 64

static Mix_Chunk *music_chunk = NULL;
static int music_playing = 0;

static ALCdevice *alc_dev = NULL;
static ALCcontext *alc_ctx = NULL;
static ALuint al_src[SFX_CHANNEL_COUNT];

#define SFX_CLIP_INVALID_ID ((unsigned)-1)

struct clip {
	unsigned res_id;
	void *data;
	size_t size;
	struct wave wave;
};

static struct sfx_heap {
	struct clip *clips;
	unsigned count, capacity;
} sfx_heap;

static void sfx_heap_init(struct sfx_heap *h)
{
	h->clips = NULL;
	h->count = h->capacity = 0;
}

static int sfx_heap_create(struct sfx_heap *h, unsigned capacity)
{
	struct clip *data = malloc(capacity * sizeof *data);
	if (!data)
		show_oem(1);

	h->clips = data;
	h->count = 0;
	h->capacity = capacity;
	return 0;
}

static void sfx_heap_free(struct sfx_heap *h)
{
	if (h->clips)
		free(h->clips);
	sfx_heap_init(h);
}

#define heap_parent(x) (((x)-1)/2)
#define heap_right(x) (2*((x)+1))
#define heap_left(x) (heap_right(x)-1)

static void sfx_heap_dump(const struct sfx_heap *h)
{
	for (unsigned i = 0, n = h->count; i < n; ++i)
		printf(" %u", h->clips[i].res_id);
	putchar('\n');
}

static void sfx_heap_swap(struct sfx_heap *h, unsigned a, unsigned b)
{
	struct clip tmp = h->clips[a];
	h->clips[a] = h->clips[b];
	h->clips[b] = tmp;
}

static void sfx_heap_sift(struct sfx_heap *h, unsigned i)
{
	unsigned l, r, m, size;

	r = heap_right(i);
	l = heap_left(i);
	m = i;
	size = h->count;

	if (l < size && h->clips[i].res_id < h->clips[i].res_id)
		m = l;
	if (r < size && h->clips[i].res_id < h->clips[i].res_id)
		m = r;

	if (m != i) {
		sfx_heap_swap(h, i, m);
		sfx_heap_sift(h, m);
	}
}

static void sfx_heap_siftup(struct sfx_heap *h, unsigned i)
{
	for (unsigned j; i > 0; i = j) {
		j = heap_parent(i);
		if (h->clips[j].res_id > h->clips[i].res_id)
			sfx_heap_swap(h, j, i);
		else
			break;
	}
}

static void sfx_heap_add(struct sfx_heap *h, struct clip *c)
{
	if (h->count >= h->capacity) {
		unsigned newcap = h->capacity << 1;
		struct clip *clips = realloc(h->clips, newcap * sizeof *clips);
		if (!clips)
			show_oem(1);
		h->capacity = newcap;
	}
	h->clips[h->count] = *c;
	sfx_heap_sift(h, h->count++);

	sfx_heap_dump(h);
}

static struct clip *sfx_heap_get(const struct sfx_heap *h, unsigned id)
{
	for (unsigned i = 0, n = h->count; i < n;) {
		unsigned res_id = h->clips[i].res_id;

		if (res_id == id)
			return &h->clips[i];
		else if (res_id > id)
			i = heap_right(i) >= n ? heap_left(i) : heap_right(i);
		else
			i = heap_left(i);
	}
	return NULL;
}

void ge_msc_stop(void)
{
	if (music_playing) {
		Mix_HaltChannel(MUSIC_CHANNEL);
		music_playing = 0;
	}
	if (music_chunk) {
		Mix_FreeChunk(music_chunk);
		music_chunk = NULL;
	}
}

static int sfx_init_al(void)
{
	const char *dname = NULL;
	int error = 1;

	if (alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT"))
		dname = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
	alc_dev = alcOpenDevice(dname);
	if (!alc_dev) {
		show_error("OpenAL sound failed", "Could not open device");
		goto fail;
	}
	alc_ctx = alcCreateContext(alc_dev, NULL);
	if (!alc_ctx) {
		show_error("OpenAL sound failed", "Could not create context");
		goto fail;
	}
	if (!alcMakeContextCurrent(alc_ctx)) {
		show_error("OpenAL sound failed", "Cannot attach audio context");
		goto fail;
	}
	memset(al_src, 0, sizeof al_src);
	alGenSources(SFX_CHANNEL_COUNT, al_src);
	sfx_init |= SFX_INIT_SRC;

	error = 0;
fail:
	return error;
}

static void sfx_free_al(void)
{
	if (sfx_init & SFX_INIT_SRC) {
		alDeleteSources(SFX_CHANNEL_COUNT, al_src);
		sfx_init &= ~SFX_INIT_SRC;
	}
	if (alc_ctx) {
		alcDestroyContext(alc_ctx);
		alc_ctx = NULL;
	}
	if (alc_dev) {
		alcCloseDevice(alc_dev);
		alc_dev = NULL;
	}
}

void ge_sfx_free(void)
{
	if (!sfx_init)
		return;
	sfx_init &= ~SFX_INIT;

	ge_msc_stop();

	if (sfx_init & SFX_INIT_HEAP) {
		sfx_heap_free(&sfx_heap);
		sfx_init &= ~SFX_INIT_HEAP;
	}
	if (sfx_init & SFX_INIT_DEV) {
		Mix_CloseAudio();
		sfx_init &= ~SFX_INIT_DEV;
	}
	sfx_free_al();
	Mix_Quit();

	if (sfx_init)
		warnx(
			"%s: expected state to be zero, but got %d",
			__func__, sfx_init
		);
}

int ge_sfx_init(void)
{
	int error = 1;

	if (sfx_init) {
		warnx("%s: already initialized", __func__);
		return 0;
	}
	sfx_heap_init(&sfx_heap);
	sfx_init = SFX_INIT;

	int flags = MIX_INIT_OGG;
	if ((Mix_Init(flags) & flags) != flags) {
		fprintf(stderr, "SDL2 mixer failed to initialize ogg support: %s\n", Mix_GetError());
		goto fail;
	}
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) == -1) {
		fprintf(stderr, "SDL2 mixer failed to open audio: %s\n", Mix_GetError());
		goto fail;
	}
	if (Mix_AllocateChannels(MUSIC_CHANNEL_COUNT) < MUSIC_CHANNEL_COUNT) {
		fprintf(stderr, "SDL2 mixer failed to allocate %d channels: %s\n", MUSIC_CHANNEL_COUNT, Mix_GetError());
		goto fail;
	}
	sfx_init |= SFX_INIT_DEV;

	error = sfx_init_al();
	if (error)
		goto fail;
	error = sfx_heap_create(&sfx_heap, SFX_HEAP_DEFAULT_SIZE);
	if (error)
		goto fail;

	error = 0;
fail:
	return error;
}

int ge_msc_play(unsigned id, int loops)
{
	if (genie_mode & GENIE_MODE_NOMUSIC)
		return 0;

	const char *path = ge_cdrom_get_music_path(id);
	if (!path)
		return 1;
	music_chunk = Mix_LoadWAV(path);
	if (!music_chunk)
		goto fail;

	int channel = Mix_PlayChannel(MUSIC_CHANNEL, music_chunk, loops);
	if (channel == -1) {
fail:
		fprintf(stderr, "music playback failed for \"%s\": %s\n", path, Mix_GetError());
		return 1;
	}
	if (channel != MUSIC_CHANNEL)
		warnx("music started on wrong channel: %d (expected: %d)\n", channel, MUSIC_CHANNEL);
	return 0;
}

static ALuint get_free_channel(void)
{
	unsigned free;
	for (free = 0; free < SFX_CHANNEL_COUNT; ++free) {
		ALint state;
		alGetSourcei(al_src[free], AL_SOURCE_STATE, &state);
		if (state != AL_PLAYING)
			break;
	}
	if (free == SFX_CHANNEL_COUNT)
		--free; /* just take the last one if everything is full */
	return al_src[free];
}

int ge_sfx_play(unsigned id)
{
	void *data;
	size_t size;
	off_t pos;
	struct clip c, *clip;
	struct wave wave;
	ALuint src;

	clip = sfx_heap_get(&sfx_heap, id);
	if (clip)
		goto play;
	dbgf("sfx: fetch id=%u\n", id);
	data = drs_get_item(DT_WAVE, id, &size, &pos);
	if (!data)
		return 1;
	clip = &c;
	clip->res_id = id;
	clip->data = data;
	clip->size = size;
	if (wave_load(&clip->wave, clip->data, clip->size))
		return 1;
	sfx_heap_add(&sfx_heap, clip);
play:
	wave = clip->wave;
	src = get_free_channel();
	ALint wave_size;
	alGetBufferi(wave.buf, AL_SIZE, &wave_size);
	dbgf("sfx: play id=%u on %u (size=%d)\n", id, src, wave_size);
	alSourcei(src, AL_BUFFER, wave.buf);
	alSourcei(src, AL_LOOPING, AL_FALSE);
	alSourcef(src, AL_GAIN, 1.0f);
	alSourcePlay(src);
	return 0;
}
