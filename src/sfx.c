#include "sfx.h"
#include "../dbg.h"
#include "todo.h"
#include "prompt.h"
#include "configure.h"
#include <AL/alc.h>
#include <AL/al.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../genie/drs.h"
#include "../genie/dmap.h"
#if HAS_MIDI
#include <fluidsynth.h>
#include <dirent.h>

static fluid_settings_t *settings;
static fluid_synth_t *synth;
static fluid_player_t *player;
static fluid_audio_driver_t *driver;

#ifndef MIDI_SF_DIR
#define MIDI_SF_DIR "/usr/share/sounds/sf2"
#endif

#endif

#define AL_NSRC 20
#define AL_NBUF 20

static ALCdevice *aldev = NULL;
static ALCcontext *ctx = NULL;
static ALuint src[AL_NSRC], src_i;
static ALuint buf[AL_NBUF], buf_i;

#define INIT_SFX 1
#define INIT_MIDI 2
#define INIT_SF 4
#define INIT_SFX_AL_DEV 8
#define INIT_SFX_AL_CTX 16
#define INIT_SFX_SRC 32

static unsigned init = 0;

static const char *msctbl[] = {
	[MUSIC_MAIN  ] = "sound/OPEN.MID"    ,
	[MUSIC_BKG1  ] = "sound/MUSIC1.MID"  ,
	[MUSIC_BKG2  ] = "sound/MUSIC2.MID"  ,
	[MUSIC_BKG3  ] = "sound/MUSIC3.MID"  ,
	[MUSIC_BKG4  ] = "sound/MUSIC4.MID"  ,
	[MUSIC_BKG5  ] = "sound/MUSIC5.MID"  ,
	[MUSIC_BKG6  ] = "sound/MUSIC6.MID"  ,
	[MUSIC_BKG7  ] = "sound/MUSIC7.MID"  ,
	[MUSIC_BKG8  ] = "sound/MUSIC8.MID"  ,
	[MUSIC_BKG9  ] = "sound/MUSIC9.MID"  ,
	[MUSIC_WON   ] = "sound/WON.MID"     ,
	[MUSIC_LOST  ] = "sound/LOST.MID"    ,
	[MUSIC_XMAIN ] = "sound/xopen.mid"   ,
	[MUSIC_XBKG1 ] = "sound/xmusic1.mid" ,
	[MUSIC_XBKG2 ] = "sound/xmusic2.mid" ,
	[MUSIC_XBKG3 ] = "sound/xmusic3.mid" ,
	[MUSIC_XBKG4 ] = "sound/xmusic4.mid" ,
	[MUSIC_XBKG5 ] = "sound/xmusic5.mid" ,
	[MUSIC_XBKG6 ] = "sound/xmusic6.mid" ,
	[MUSIC_XBKG7 ] = "sound/xmusic7.mid" ,
	[MUSIC_XBKG8 ] = "sound/xmusic8.mid" ,
	[MUSIC_XBKG9 ] = "sound/xmusic9.mid" ,
	[MUSIC_XBKG10] = "sound/xmusic10.mid",
	[MUSIC_XWON  ] = "sound/xwon.mid"    ,
	[MUSIC_XLOST ] = "sound/xlost.mid"   ,
};

#define MSCTBLSZ (sizeof(msctbl)/sizeof(msctbl[0]))

int clip4A3440(struct clip *this)
{
	stub
	(void)this;
	return 0;
}

int clip_ctl(struct clip *this)
{
	stub
	(void)this;
	return 0;
}

#if HAS_MIDI
static void midi_free(void)
{
	if (player)
		fluid_player_stop(player);
	if (driver) {
		delete_fluid_audio_driver(driver);
		driver = NULL;
	}
	if (player) {
		delete_fluid_player(player);
		player = NULL;
	}
	if (synth) {
		delete_fluid_synth(synth);
		synth = NULL;
	}
	if (settings) {
		delete_fluid_settings(settings);
		settings = NULL;
	}
	init &= ~INIT_SF;
}

static int midi_init(void)
{
	const char *err = "Unknown error";
	char path[1024];
	struct dirent **list = NULL;
	int n = 0, ret = 1;
	if (!(settings = new_fluid_settings())) {
		err = "No midi settings";
		goto fail;
	}
	if (!(synth = new_fluid_synth(settings))) {
		err = "No midi synthesizer";
		goto fail;
	}
	fluid_settings_setstr(settings, "audio.driver", "alsa");
	if (!(player = new_fluid_player(synth))) {
		err = "No midi player";
		goto fail;
	}
	if (!(driver = new_fluid_audio_driver(settings, synth))) {
		err = "No midi audio driver";
		goto fail;
	}
	n = scandir(MIDI_SF_DIR, &list, NULL, alphasort);
	if (n <= 0) {
no_sf:
		show_error("No music", "No soundfont installed");
		// TODO ask for soundfont
	} else {
		for (int i = 0; i < n; ++i) {
			snprintf(path, sizeof path, "%s/%s", MIDI_SF_DIR, list[i]->d_name);
			if (fluid_is_soundfont(path)) {
				dbgf("using %s\n", list[i]->d_name);
				fluid_synth_sfload(synth, path, 1);
				init |= INIT_SF;
				goto sf;
			}
		}
		goto no_sf;
	}
sf:
	ret = 0;
fail:
	if (list) {
		while (n--)
			free(list[n]);
		free(list);
	}
	if (ret) {
		midi_free();
		show_error("No music", err);
	}
	return ret;
}
#endif

int midi_play(unsigned id)
{
	const char *clipname = msctbl[id];
#if HAS_MIDI
	if (!(init & INIT_SF)) {
		dbgf("no soundfont\nignore midi playback: %s\n", clipname);
		return -1;
	}
	if (!fluid_is_midifile(clipname)) {
		char buf[256];
		sprintf(buf, "Not a midi file: %s\n", clipname);
		show_error("Audio error", buf);
		return 1;
	}
	if (fluid_player_get_status(player) == FLUID_PLAYER_PLAYING)
		fluid_player_stop(player);
	fluid_player_add(player, clipname);
	fluid_player_play(player);
	return 0;
#else
	dbgf("ignore midi playback: %s\n", clipname);
	return -1;
#endif
}

int sfx_init(void)
{
	if (init & INIT_SFX)
		return 0;
	init |= INIT_SFX;
#if HAS_MIDI
	if (midi_init())
		return 1;
	init |= INIT_MIDI;
#endif
	const char *devname = NULL;
#if AL_DEBUG
	if (alcIsExtensionPresent(NULL, "ALC_NUMERATION_EXT") == AL_TRUE) {
		puts("audio devices:");
		const ALCchar *dev, *next;
		devname = dev = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
		next = dev + 1;
		size_t len = 0;
		while (dev && *dev && next && *next) {
			puts(dev);
			len = strlen(dev);
			dev += len + 1;
			next += len + 2;
		}
	}
#endif
	if (!devname)
		devname = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
	if (!(aldev = alcOpenDevice(devname))) {
		fputs("no audio device\n", stderr);
		return 1;
	}
	init |= INIT_SFX_AL_DEV;
	if (!(ctx = alcCreateContext(aldev, NULL))) {
		fputs("no audio context\n", stderr);
		return 1;
	}
	if (!alcMakeContextCurrent(ctx)) {
		fputs("cannot attach audio context\n", stderr);
		return 1;
	}
#if AL_DEBUG
	printf("audio device: %s\n", alcGetString(aldev, ALC_DEVICE_SPECIFIER));
#endif
	src[AL_NSRC - 1] = 0;
	alGenSources(AL_NSRC, src);
	if (!src[AL_NSRC - 1]) {
		show_error("Audio error", "Not enough channels");
		exit(1);
	}
	buf[AL_NBUF - 1] = 0;
	alGenBuffers(AL_NBUF, buf);
	if (!buf[AL_NBUF - 1]) {
		show_error("Audio error", "Not enough buffers");
		exit(1);
	}
	src_i = buf_i = 0;
	init |= INIT_SFX_SRC;
	return 0;
}

void sfx_free(void)
{
	if (!(init & INIT_SFX))
		return;
	if (init & INIT_SFX_SRC) {
		alDeleteBuffers(AL_NBUF, buf);
		alDeleteSources(AL_NSRC, src);
		init &= ~INIT_SFX_SRC;
	}
	if (ctx) {
		if (aldev)
			alcMakeContextCurrent(NULL);
		alcDestroyContext(ctx);
		ctx = NULL;
		init &= ~INIT_SFX_AL_CTX;
	}
	if (aldev) {
		alcCloseDevice(aldev);
		aldev = NULL;
		init &= ~INIT_SFX_AL_DEV;
	}
#if HAS_MIDI
	midi_free();
	init &= ~INIT_MIDI;
#endif
	init &= ~INIT_SFX;
}

#define RIFF_MAGIC   0x46464952
#define WAVE_MAGIC   0x45564157
#define FORMAT_MAGIC 0x20746d66
#define DATA_MAGIC   0x61746164

struct ckfmt {
	uint16_t format;
	uint16_t channels;
	uint32_t freq;
	uint32_t bytes;
	uint16_t align;
	uint16_t sample;
};

static int wave_load_format(const void *data, struct ckfmt *format, size_t *wavesz, const void **wavedata, unsigned offset, size_t size)
{
	uint32_t *magic = (uint32_t*)data;
	if (*magic != RIFF_MAGIC) {
		fprintf(stderr, "bad riff at %x: magic=%x\n", offset, *magic);
		return 1;
	}
	if (magic[2] != WAVE_MAGIC) {
		fprintf(stderr, "bad wave at %x: magic=%x\n", offset + 4, magic[2]);
		return 1;
	}
	struct ckfmt *fmt = NULL;
	const void *ckdata = NULL;
	size_t datasz = 0;
	for (uint32_t c_off = 12; c_off < magic[1];) {
		if (c_off > size)
			goto bad_wave;
		uint32_t *c_data = (uint32_t*)((char*)data + c_off);
		uint32_t cksz = c_data[1];
		if (!cksz) {
			fprintf(stderr, "empty chunk at %x\n", offset + c_off);
			return 1;
		}
		if (c_data[0] == FORMAT_MAGIC) {
			if (cksz < 16) {
				fprintf(stderr, "bad format chunk at %x: size=%x\n", offset + c_off, cksz);
				return 1;
			}
			if (fmt)
				fprintf(stderr, "overriding format at %x\n", offset + c_off);
			fmt = (struct ckfmt*)((char*)data + c_off + 8);
		} else if (c_data[0] == DATA_MAGIC) {
			if (ckdata)
				fprintf(stderr, "overriding data at %x\n", offset + c_off);
			ckdata = (char*)data + c_off;
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
	if (datasz > size) {
bad_wave:
		fprintf(stderr, "bad wave at %x\n", offset);
		return 1;
	}
	*format = *fmt;
	*wavesz = datasz;
	*wavedata = (char*)ckdata + 8;
	return 0;
}

static ALuint free_channel(unsigned *index)
{
	for (unsigned free = 0; free < AL_NSRC; ++free) {
		ALint state;
		alGetSourcei(src[free], AL_SOURCE_STATE, &state);
		if (state != AL_PLAYING) {
			*index = free;
			return src[free];
		}
	}
	unsigned ch = src[src_i];
	*index = src_i;
	alSourceStop(ch);
	src_i = (src_i + 1) % AL_NSRC;
	return ch;
}

static int wave_play(const void *data, off_t offset, size_t size, unsigned flags, float volume)
{
	struct ckfmt fmt;
	size_t wavesz;
	const void *wavedata;
	ALenum format;
	if (wave_load_format(data, &fmt, &wavesz, &wavedata, offset, size))
		goto fail;
	switch (fmt.format) {
	case 1:
	case 7:
		break;
	default:
		fprintf(stderr, "unknown format %u\n", fmt.format);
		goto fail;
	}
	switch (fmt.channels) {
	case 1:
		switch (fmt.sample) {
		case  8: format = AL_FORMAT_MONO8 ; break;
		case 16: format = AL_FORMAT_MONO16; break;
		default:
			fprintf(stderr, "bad sample size %u\n", fmt.sample);
			goto fail;
		}
		break;
	case 2:
		switch (fmt.sample) {
		case  8: format = AL_FORMAT_STEREO8 ; break;
		case 16: format = AL_FORMAT_STEREO16; break;
		default:
			fprintf(stderr, "bad sample size %u\n", fmt.sample);
			goto fail;
		}
		break;
	default:
		fprintf(stderr, "bad channel count: %u\n", fmt.channels);
		goto fail;
	}
	unsigned index;
	ALuint ch = free_channel(&index);
	alBufferData(buf[index], format, wavedata, wavesz, fmt.freq);
	alSourcei(ch, AL_BUFFER, buf[index]);
	alSourcei(ch, AL_LOOPING, flags & SFX_PLAY_LOOP ? AL_TRUE : AL_FALSE);
	alSourcef(ch, AL_GAIN, volume);
	alSourcePlay(ch);
	return 0;
fail:
	return 1;
}

int sfx_play(unsigned id, unsigned flags, float volume)
{
	char buf[256];
	if (id >= MSCTBLSZ) {
		// not a music file, try to find a clip
		if (id > UINT16_MAX)
			goto bad_id;
		size_t count;
		off_t offset;
		const void *drs_item = drs_get_item(DT_WAVE, id, &count, &offset);
		if (!drs_item || wave_play(drs_item, offset, count, flags, volume))
			goto bad_id;
		return 0;
bad_id:
		sprintf(buf, "Bad id: %u\n", id);
		show_error("Audio error", buf);
		return 1;
	}
	return midi_play(id);
}
