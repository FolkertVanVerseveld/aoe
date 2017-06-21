#include "prompt.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "tinyfiledialogs.h"
#include "ui.h"

void show_oem(int code)
{
	show_error("Fatal error", "Out of memory");
	exit(code);
}

void show_error(const char *title, const char *msg)
{
	if (title)
		fprintf(stderr, "%s: %s\n", title, msg);
	else
		fprintf(stderr, "%s\n", msg);

	if (genie_ui_is_visible(&genie_ui) || !isatty(fileno(stdin)))
		show_message(title, msg, BTN_OK, MSG_ERR, BTN_OK);
}

int show_message(const char *title, const char *message, unsigned button, unsigned type, unsigned defbtn)
{
	int ret;
	const char *tfd_type, *tfd_btn;
	unsigned tfd_def;

	if (!title)
		title = "(null)";

	if (!message)
		message = "(null)";

	switch (button) {
	case BTN_OKCANCEL: tfd_btn = "okcancel"; break;
	case BTN_OK      : tfd_btn =       "ok"; break;
	case BTN_YESNO   : tfd_btn =    "yesno"; break;
	default:
		return -1;
	}

	switch (type) {
	case MSG_INFO: tfd_type =     "info"; break;
	case MSG_WARN: tfd_type =     "warn"; break;
	case MSG_ASK : tfd_type = "question"; break;
	case MSG_ERR : tfd_type =    "error"; break;
	default:
		return -1;
	}

	switch (defbtn) {
	case BTN_NO : tfd_def = 0; break;
	case BTN_YES: tfd_def = 1; break;
	default:
		return -1;
	}

	ret = tinyfd_messageBox(title, message, tfd_btn, tfd_type, tfd_def);

	switch (ret) {
	case 0: return  BTN_NO;
	case 1: return BTN_YES;
	default:
		fprintf(stderr, "prompt: unexpected return value: %d\n", ret);
		return -1;
	}
}

char *prompt_input(const char *title, const char *message, const char *input)
{
	return tinyfd_inputBox(title, message, input);
}

char *prompt_file(const char *title, const char *defpath, unsigned nfilter, const char **filter, const char *desc, unsigned flags)
{
	int multi_select;

	if (!(flags & (PROMPT_LOAD | PROMPT_SAVE)))
		return NULL;

	multi_select = flags & PROMPT_MULTISELECT ? 1 : 0;

	if (flags & PROMPT_LOAD)
		return tinyfd_openFileDialog(
			title, defpath,
			nfilter, filter, desc,
			multi_select
		);
	else
		return tinyfd_saveFileDialog(
			title, defpath,
			nfilter, filter, desc
		);
}

char *prompt_folder(const char *title, const char *defpath)
{
	return tinyfd_selectFolderDialog(title, defpath);
}
