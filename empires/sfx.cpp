#include "sfx.h"

#include "../setup/def.h"

#define SFX_CHANNEL_COUNT 20

Mix_Chunk *music_chunk = NULL;
int music_playing = 0;

ALCdevice *alc_dev = NULL;
ALCcontext *alc_ctx = NULL;
ALuint al_src[SFX_CHANNEL_COUNT];

extern "C" {

void sfx_init(void)
{
	char buf[256];

	int flags = MIX_INIT_OGG;
	if ((Mix_Init(flags) & flags) != flags) {
		snprintf(buf, sizeof buf, "SDL2 mixer failed to initialize ogg support: %s\n", Mix_GetError());
		panic(buf);
	}
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) == -1) {
		snprintf(buf, sizeof buf, "SDL2 mixer failed to open audio: %s\n", Mix_GetError());
		panic(buf);
	}
	if (Mix_AllocateChannels(MUSIC_CHANNEL_COUNT) < MUSIC_CHANNEL_COUNT) {
		snprintf(buf, sizeof buf, "SDL2 mixer failed to allocate %d channels: %s\n", MUSIC_CHANNEL_COUNT, Mix_GetError());
		panic(buf);
	}
}

void sfx_free(void)
{
	Mix_Quit();
}

}
