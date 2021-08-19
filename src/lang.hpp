#pragma once

#include <cstdarg>
#include <string>

enum class LangId {
	standard,
	english = 0, // English: English
	//zhongwen,    // Chinese: 中文
	nederlands,  // Dutch: Nederlands
	deutsch,     // German: Deutsch
	//nihongo,     // Japanese: 日本語
	//espanol,     // Spanish: Español
	//turk,        // Turkish: Türk
	//tieng_viet,  // Vietnamese: Tiếng Việt
	max, // end marker
};

enum class TextId {
	// debug stuff
	wip,
	// general purpose
	title,
	language,
	hello,
	about,
	set_game_dir,
	dlg_game_dir,
	dlg_global,
	// buttons
	btn_quit,
	btn_back,
	btn_about,
	btn_startup,
	// main menu
	btn_singleplayer,
	btn_multiplayer,
	btn_main_settings,
	btn_editor,
	// single player menu
	title_singleplayer,
	btn_random_map,
	btn_missions,
	btn_deathmatch,
	btn_scenario,
	btn_restore_game,
	btn_cancel,
	// single player game
	title_singleplayer_game,
	btn_game_settings,
	btn_start_game,
	btn_tooltip,
	player_name,
	player_civ,
	player_id,
	player_team,
	player_count,
	// editor
	btn_scn_edit,
	btn_cpn_edit,
	btn_drs_edit,
	// scn editor
	btn_scn_load,
	// settings
	cfg_sfx,
	cfg_sfx_vol,
	cfg_msc,
	cfg_msc_vol,
	cfg_fullscreen,
	cfg_center_window,
	cfg_display_mode,
	dim_custom,
	// tasks
	work_drs_success,
	work_drs, // check_root: find drs
	work_load_menu,
	// errors
	err_drs_path,
	err_drs_hdr,
	err_drs_empty,
	err_no_drs_item,
	err_no_ttf,
	max,
};

extern const char *langs[];

extern LangId lang;

const std::string &TXT(LangId lang, TextId id);
static const std::string &TXT(TextId id) { return TXT(lang, id); }

static const char *CTXT(LangId lang, TextId id) { return TXT(lang, id).c_str(); }
static const char *CTXT(TextId id) { return TXT(id).c_str(); }

std::string TXTF(LangId lang, TextId id, ...);
std::string TXTF(LangId lang, TextId id, va_list);

static std::string TXTF(TextId id, ...)
{
	va_list args;
	va_start(args, id);
	std::string s(TXTF(lang, id, args));
	va_end(args);
	return s;
}
