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
	title,
	language,
	hello,
	about,
	set_game_dir,
	dlg_game_dir,
	// buttons
	btn_quit,
	btn_back,
	btn_about,
	btn_startup,
	btn_editor,
	// tasks
	work_drs_success,
	work_drs, // check_root: find drs
	work_load_menu,
	// errors
	err_drs_path,
	err_drs_hdr,
	err_drs_empty,
	err_no_drs_item,
	max,
};

extern const char *langs[];

extern LangId lang;

const char *TXT(LangId lang, TextId id);
static const char *TXT(TextId id) { return TXT(lang, id); }

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
