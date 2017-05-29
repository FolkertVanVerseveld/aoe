#include "prompt.h"
#include <stdio.h>
#include "tinyfiledialogs.h"

int show_message(const char *title, const char *message, unsigned button, unsigned type, unsigned defbtn)
{
	const char *tfd_type, *tfd_btn;
	unsigned tfd_def;
	if (!title) title = "(null)";
	if (!message) message = "(null)";
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
	int ret = tinyfd_messageBox(title, message, tfd_btn, tfd_type, tfd_def);
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
	if (!(flags & (PROMPT_LOAD | PROMPT_SAVE)))
		return NULL;
	return flags & PROMPT_LOAD ? tinyfd_openFileDialog(title, defpath, nfilter, filter, desc, flags & PROMPT_MULTISELECT ? 1 : 0)
		: tinyfd_saveFileDialog(title, defpath, nfilter, filter, desc);
}

char *prompt_folder(const char *title, const char *defpath)
{
	return tinyfd_selectFolderDialog(title, defpath);
}
