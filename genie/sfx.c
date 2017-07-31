/* Copyright 2016-2017 the Age of Empires Free Software Remake authors. See LEGAL for legal info */

#include "sfx.h"
#include "engine.h"

#include <stdio.h>
#include <err.h>

#include <SDL2/SDL_mixer.h>

#define SFX_INIT 1
#define SFX_INIT_DEV 2

static int sfx_init = 0;

#define MUSIC_CHANNEL 0

#define CHANNEL_COUNT 20

static Mix_Chunk *music_chunk = NULL;
static int music_playing = 0;

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

void ge_sfx_free(void)
{
	if (!sfx_init)
		return;
	sfx_init &= ~SFX_INIT;

	ge_msc_stop();
	if (sfx_init & SFX_INIT_DEV) {
		Mix_CloseAudio();
		sfx_init &= ~SFX_INIT_DEV;
	}
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
	if (Mix_AllocateChannels(CHANNEL_COUNT) < CHANNEL_COUNT) {
		fprintf(stderr, "SDL2 mixer failed to allocate %d channels: %s\n", CHANNEL_COUNT, Mix_GetError());
		goto fail;
	}
	sfx_init |= SFX_INIT_DEV;

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
	printf("music path to play: %s\n", path);
	music_chunk = Mix_LoadWAV(path);
	if (!music_chunk) {
		goto fail;
	}
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
