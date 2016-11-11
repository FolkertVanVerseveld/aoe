#ifndef AOE_PROMPT_H
#define AOE_PROMPT_H

#define MSG_INFO 0
#define MSG_ASK 1
#define MSG_WARN 2
#define MSG_ERR 3

#define BTN_CANCEL 0
#define BTN_NO 0
#define BTN_YES 1
#define BTN_OK 1
#define BTN_OKCANCEL 2
#define BTN_YESNO 3

int show_message(const char *title, const char *message, unsigned button, unsigned type, unsigned defbtn);
void show_error(const char *title, const char *msg);
char *prompt_input(const char *title, const char *message, const char *input);

#define PROMPT_LOAD 1
#define PROMPT_SAVE 2
#define PROMPT_MULTISELECT 4

char *prompt_file(const char *title, const char *defpath, unsigned nfilter, const char **filter, const char *desc, unsigned flags);
char *prompt_folder(const char *title, const char *defpath);

#endif
