#include "sfx.h"
#include "dbg.h"
#include "todo.h"
#include "prompt.h"
#include "configure.h"
#if HAS_MIDI
#include <stdlib.h>
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

#define INIT_SFX 1
#define INIT_MIDI 2
#define INIT_SF 4

static unsigned init = 0;

#define SFX_MIDI 1

static const struct sfx {
	const char *name;
	unsigned flags;
} sfxtbl[] = {
	[MUSIC_MAIN  ] = {"sound/OPEN.MID"    , SFX_MIDI},
	[MUSIC_BKG1  ] = {"sound/MUSIC1.MID"  , SFX_MIDI},
	[MUSIC_BKG2  ] = {"sound/MUSIC2.MID"  , SFX_MIDI},
	[MUSIC_BKG3  ] = {"sound/MUSIC3.MID"  , SFX_MIDI},
	[MUSIC_BKG4  ] = {"sound/MUSIC4.MID"  , SFX_MIDI},
	[MUSIC_BKG5  ] = {"sound/MUSIC5.MID"  , SFX_MIDI},
	[MUSIC_BKG6  ] = {"sound/MUSIC6.MID"  , SFX_MIDI},
	[MUSIC_BKG7  ] = {"sound/MUSIC7.MID"  , SFX_MIDI},
	[MUSIC_BKG8  ] = {"sound/MUSIC8.MID"  , SFX_MIDI},
	[MUSIC_BKG9  ] = {"sound/MUSIC9.MID"  , SFX_MIDI},
	[MUSIC_WON   ] = {"sound/WON.MID"     , SFX_MIDI},
	[MUSIC_LOST  ] = {"sound/LOST.MID"    , SFX_MIDI},
	[MUSIC_XMAIN ] = {"sound/xopen.mid"   , SFX_MIDI},
	[MUSIC_XBKG1 ] = {"sound/xmusic1.mid" , SFX_MIDI},
	[MUSIC_XBKG2 ] = {"sound/xmusic2.mid" , SFX_MIDI},
	[MUSIC_XBKG3 ] = {"sound/xmusic3.mid" , SFX_MIDI},
	[MUSIC_XBKG4 ] = {"sound/xmusic4.mid" , SFX_MIDI},
	[MUSIC_XBKG5 ] = {"sound/xmusic5.mid" , SFX_MIDI},
	[MUSIC_XBKG6 ] = {"sound/xmusic6.mid" , SFX_MIDI},
	[MUSIC_XBKG7 ] = {"sound/xmusic7.mid" , SFX_MIDI},
	[MUSIC_XBKG8 ] = {"sound/xmusic8.mid" , SFX_MIDI},
	[MUSIC_XBKG9 ] = {"sound/xmusic9.mid" , SFX_MIDI},
	[MUSIC_XBKG10] = {"sound/xmusic10.mid", SFX_MIDI},
	[MUSIC_XWON  ] = {"sound/xwon.mid"    , SFX_MIDI},
	[MUSIC_XLOST ] = {"sound/xlost.mid"   , SFX_MIDI},
};

#define SFXTBLSZ (sizeof(sfxtbl)/sizeof(sfxtbl[0]))

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
	const struct sfx *clip = &sfxtbl[id];
#if HAS_MIDI
	if (!(init & INIT_SF)) {
		dbgf("no soundfont\nignore midi playback: %s\n", clip->name);
		return -1;
	}
	if (!(clip->flags & SFX_MIDI) || !fluid_is_midifile(clip->name)) {
		char buf[256];
		sprintf(buf, "Not a midi file: %s\n", clip->name);
		show_error("Audio error", buf);
		return 1;
	}
	fluid_player_add(player, clip->name);
	fluid_player_play(player);
	return 0;
#else
	dbgf("ignore midi playback: %s\n", clip->name);
	return -1;
#endif
}

int sfx_init(void)
{
	if (init & INIT_SFX)
		return 0;
#if HAS_MIDI
	if (midi_init())
		return 1;
	init |= INIT_MIDI;
#endif
	init |= INIT_SFX;
	return 0;
}

void sfx_free(void)
{
	if (!(init & INIT_SFX))
		return;
#if HAS_MIDI
	midi_free();
	init &= ~INIT_MIDI;
#endif
	init &= ~INIT_SFX;
}

int sfx_play(unsigned id)
{
	char buf[256];
	if (id >= SFXTBLSZ) {
		sprintf(buf, "Bad id: %u\n", id);
		show_error("Audio error", buf);
		return 1;
	}
	const struct sfx *clip = &sfxtbl[id];
	if (clip->flags & SFX_MIDI)
		return midi_play(id);
	dbgf("ignore playback: %s\n", clip->name);
	return -1;
}
